#include "src/arrow.h"
#include <arrow/api.h>
#include <arrow/compute/exec/exec_plan.h>
#include <arrow/dataset/file_parquet.h>
#include <arrow/filesystem/filesystem.h>
#include <folly/Expected.h>
#include <folly/logging/xlog.h>
#include <iostream>
#include <memory>
#include <parquet/arrow/writer.h>
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
get_fs_dataset(const std::string &root_path) {
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
    XLOG(INFO) << "Found fragment: " << (*fragment)->ToString() << std::endl;
  }

  return dataset;
}
} // namespace

/*static*/
folly::Expected<std::unique_ptr<BapidTable>, std::string>
BapidTable::fromFsDataset(const std::string &dataset_dir, std::string name) {
  auto dataset = get_fs_dataset(dataset_dir);
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

SamplesQuery &SamplesQuery::filter() {
  filters_.emplace_back(
      cp::greater(cp::field_ref("total_amount"), cp::literal(30)));
  return *this;
}

SamplesQuery &SamplesQuery::project() {
  projects_.emplace_back(cp::field_ref("total_amount"));
  result_set_schema_.emplace_back(
      arrow::field("total_amount", arrow::float64()));
  return *this;
}

folly::Expected<SamplesQuery::RunnableQuery, std::string>
SamplesQuery::finalize() && {
  // TODO(liuz): projection for scanner and validation
  auto options = std::make_shared<ds::ScanOptions>();
  options->projection = cp::project({}, {});
  arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen;

  decls_.emplace_back(cp::Declaration{"scan", ds::ScanNodeOptions{
                                                  dataset_,
                                                  options,
                                              }});
  for (auto &filter : filters_) {
    decls_.emplace_back(
        cp::Declaration{"filter", cp::FilterNodeOptions{filter}});
  }

  decls_.emplace_back(
      cp::Declaration{"project", cp::ProjectNodeOptions{projects_}});
  decls_.emplace_back(cp::Declaration{"sink", cp::SinkNodeOptions{&sink_gen}});

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

  auto future = plan->finished();
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

void test_arrow() {
  auto table = BapidTable::fromFsDataset(FLAGS_dataset_dir, "taxi");
  XCHECK(table.hasValue());
  auto query = table.value()->newSamplesQueryX().filter().project().take(2);
  auto result_set = std::move(query).finalize().value().gen().value();
  std::cout << "Results : " << result_set->ToString() << std::endl;
}
} // namespace bapid