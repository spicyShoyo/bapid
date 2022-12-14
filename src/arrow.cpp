#include "src/arrow.h"
#include "if/bapid.pb.h"
#include <algorithm>
#include <arrow/api.h>
#include <arrow/compute/exec/exec_plan.h>
#include <arrow/dataset/file_parquet.h>
#include <arrow/filesystem/filesystem.h>
#include <folly/Expected.h>
#include <folly/logging/xlog.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <parquet/arrow/writer.h>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace bapid {

DEFINE_string(dataset_dir, "", "dataset dir"); // NOLINT
DEFINE_string(out_dir, "", "out dir");         // NOLINT

namespace {
namespace fs = arrow::fs;
namespace ds = arrow::dataset;
namespace pq = parquet;
namespace cp = arrow::compute;

arrow::Result<std::shared_ptr<ds::Dataset>>
getFsDataset(const std::string &root_path) {
  ARROW_ASSIGN_OR_RAISE(auto file_sys, fs::FileSystemFromUriOrPath(root_path));
  auto format = std::make_shared<ds::ParquetFileFormat>();

  fs::FileSelector selector;
  selector.base_dir = root_path;

  ARROW_ASSIGN_OR_RAISE(auto factory, ds::FileSystemDatasetFactory::Make(
                                          file_sys, selector, format,
                                          ds::FileSystemFactoryOptions()));
  ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());

  ARROW_ASSIGN_OR_RAISE(auto fragments, dataset->GetFragments())
  for (const auto &fragment : fragments) {
    XLOG(INFO) << "Found fragment: " << (*fragment)->ToString();
  }

  return dataset;
}

arrow::Status describeFsDatasetImpl(const std::string &root_path) {
  ARROW_ASSIGN_OR_RAISE(auto dataset, getFsDataset(root_path));
  ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
  auto maybe_scanner = scan_builder->Finish();
  ARROW_ASSIGN_OR_RAISE(auto scanner, maybe_scanner);
  ARROW_ASSIGN_OR_RAISE(auto table, scanner->ToTable());

  std::cout << "Total rows: " << table->num_rows()
            << "; Table info: " << std::endl;
  std::cout << table->Slice(0, 1)->ToString() << std::endl;
  return arrow::Status::OK();
}

} // namespace

/*static*/ folly::Expected<folly::Unit, std::string>
BapidTable::describeFsDataset(const std::string &root_path) {
  auto status = describeFsDatasetImpl(root_path);
  if (status.ok()) {
    return folly::Unit{};
  }

  return folly::makeUnexpected(status.ToString());
}

/*static*/
folly::Expected<std::unique_ptr<BapidTable>, std::string>
BapidTable::fromFsDataset(const std::string &dataset_dir, std::string name) {
  auto dataset = getFsDataset(dataset_dir);
  if (!dataset.ok()) {
    return folly::makeUnexpected(dataset.status().ToString());
  }

  return std::make_unique<BapidTable>(std::move(name),
                                      dataset.MoveValueUnsafe());
}

BapidTable::BapidTable(std::string name, std::shared_ptr<ds::Dataset> dataset)
    : name_{std::move(name)}, dataset_{std::move(dataset)} {}

SamplesQuery BapidTable::newSamplesQueryX() {
  return SamplesQuery::fromDataset(dataset_).value();
}

namespace {
arrow::Result<SamplesQuery>
samplesQueryfromDatasetImpl(std::shared_ptr<ds::Dataset> dataset) {
  auto *registry = cp::default_exec_factory_registry();
  ds::internal::InitializeScanner(registry);
  ARROW_ASSIGN_OR_RAISE(auto plan,
                        cp::ExecPlan::Make(cp::default_exec_context()));

  return SamplesQuery{registry, std::move(plan), std::move(dataset)};
}
} // namespace

/*static*/ folly::Expected<SamplesQuery, std::string>
SamplesQuery::fromDataset(std::shared_ptr<ds::Dataset> dataset) {
  auto samples_query = samplesQueryfromDatasetImpl(std::move(dataset));
  if (!samples_query.ok()) {
    return folly::makeUnexpected(samples_query.status().ToString());
  }

  return samples_query.MoveValueUnsafe();
}

SamplesQuery::SamplesQuery(cp::ExecFactoryRegistry *registry,
                           std::shared_ptr<cp::ExecPlan> plan,
                           std::shared_ptr<ds::Dataset> dataset)
    : registry_(registry), plan_{std::move(plan)}, dataset_{
                                                       std::move(dataset)} {}

namespace {
// TODO(liuz): implement
cp::Expression getArrowExpForFilter(const bapidrpc::Filter &filter) {
  switch (filter.op()) {
  case bapidrpc::FilterOp::GT:
    return cp::greater(cp::field_ref(filter.col_name()),
                       cp::literal(filter.double_vals(0)));
  default:
    throw std::runtime_error("unimplemented");
  }
}

const std::shared_ptr<arrow::DataType> &
getArrowTypeForCol(const bapidrpc::Col &col) {
  switch (col.type()) {
  case bapidrpc::ColType::DOUBLE: {
    return arrow::float64();
  }
  default:
    throw std::runtime_error("unimplemented");
  }
}
} // namespace

SamplesQuery &SamplesQuery::filter(const bapidrpc::Filter &filter) {
  filters_.emplace_back(getArrowExpForFilter(filter));
  fields_.emplace(filter.col_name());
  return *this;
}

SamplesQuery &SamplesQuery::project(const bapidrpc::Col &col) {
  projects_.push_back(cp::field_ref(col.name()));
  fields_.emplace(col.name());
  result_set_schema_.push_back(
      arrow::field(col.name(), getArrowTypeForCol(col)));
  return *this;
}

folly::Expected<SamplesQuery::RunnableQuery, std::string>
SamplesQuery::finalize() && {
  auto options = std::make_shared<ds::ScanOptions>();

  std::vector<cp::Expression> scanner_projects{};
  std::transform(fields_.begin(), fields_.end(),
                 std::back_inserter(scanner_projects),
                 [](const auto &field) { return cp::field_ref(field); });

  options->projection = cp::project(std::move(scanner_projects), {});
  arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen;

  decls_.emplace_back("scan", ds::ScanNodeOptions{
                                  dataset_,
                                  options,
                              });
  for (const auto &filter : filters_) {
    decls_.emplace_back("filter", cp::FilterNodeOptions{filter});
  }

  decls_.emplace_back("project", cp::ProjectNodeOptions{projects_});
  decls_.emplace_back("sink", cp::SinkNodeOptions{&sink_gen});

  auto result =
      cp::Declaration::Sequence(std::move(decls_)).AddToPlan(plan_.get());

  if (!result.ok()) {
    return folly::makeUnexpected(result.status().ToString());
  }

  return SamplesQuery::RunnableQuery{
      std::move(plan_), arrow::schema(std::move(result_set_schema_)),
      std::move(sink_gen), take_};
}

SamplesQuery::RunnableQuery::RunnableQuery(
    std::shared_ptr<cp::ExecPlan> plan, std::shared_ptr<arrow::Schema> schema,
    arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen,
    std::optional<int> take)
    : plan_{std::move(plan)}, schema_{std::move(schema)},
      sink_gen_{std::move(sink_gen)}, take_{take} {}

namespace {
arrow::Result<std::shared_ptr<arrow::Table>>
genImpl(const std::shared_ptr<cp::ExecPlan> &plan,
        std::shared_ptr<arrow::Schema> schema,
        arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen) {
  auto sink_reader =
      cp::MakeGeneratorReader(std::move(schema), std::move(sink_gen),
                              cp::default_exec_context()->memory_pool());

  ARROW_RETURN_NOT_OK(plan->StartProducing());
  std::shared_ptr<arrow::Table> result_set;
  ARROW_ASSIGN_OR_RAISE(result_set,
                        arrow::Table::FromRecordBatchReader(sink_reader.get()));
  plan->StopProducing();

  const auto future = plan->finished();
  ARROW_RETURN_NOT_OK(future.status());
  return result_set;
}
} // namespace

folly::Expected<std::shared_ptr<arrow::Table>, std::string>
SamplesQuery::RunnableQuery::gen() && {
  auto result_set = genImpl(plan_, std::move(schema_), std::move(sink_gen_));
  if (!result_set.ok()) {
    return folly::makeUnexpected(result_set.status().ToString());
  }

  if (!take_) {
    return result_set.MoveValueUnsafe();
  }

  return result_set.MoveValueUnsafe()->Slice(0, take_.value());
}

SamplesQuery &SamplesQuery::take(int to_take) {
  take_ = to_take;
  return *this;
}

bapidrpc::Col DBL_COL(std::string name) {
  auto col = bapidrpc::Col{};
  col.set_name(std::move(name));
  col.set_type(bapidrpc::ColType::DOUBLE);
  return col;
}

bapidrpc::Filter DBL_GT(std::string name, double val) {
  auto filter = bapidrpc::Filter{};
  filter.set_col_name(std::move(name));
  filter.set_op(bapidrpc::FilterOp::GT);
  filter.add_double_vals(val);
  return filter;
}

void test_arrow() {
  BapidTable::describeFsDataset(FLAGS_dataset_dir);

  auto table = BapidTable::fromFsDataset(FLAGS_dataset_dir, "taxi");
  XCHECK(table.hasValue());

  auto query = table.value()
                   ->newSamplesQueryX()
                   .filter(DBL_GT("tip_amount", 30))
                   .filter(DBL_GT("tolls_amount", 10))
                   .project(DBL_COL("tip_amount"))
                   .project(DBL_COL("tolls_amount"))
                   .project(DBL_COL("total_amount"))
                   .take(10);
  auto result_set = std::move(query).finalize().value().gen().value();
  std::cout << "Results : " << result_set->ToString() << std::endl;
}

} // namespace bapid
