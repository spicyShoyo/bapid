#include <arrow/api.h>
#include <arrow/compute/exec/exec_plan.h>
#include <arrow/dataset/file_parquet.h>
#include <arrow/filesystem/filesystem.h>
#include <folly/logging/xlog.h>
#include <iostream>
#include <memory>
#include <parquet/arrow/writer.h>
#include <string_view>

namespace bapid {

DEFINE_string(dataset_dir, "", "dataset dir"); // NOLINT
DEFINE_string(out_dir, "", "out dir");         // NOLINT

namespace {
namespace fs = arrow::fs;
namespace ds = arrow::dataset;
namespace pq = parquet;
namespace cp = arrow::compute;

arrow::Result<std::shared_ptr<ds::Dataset>>
get_dataset(const std::string &root_path) {
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
    std::cout << "Found fragment: " << (*fragment)->ToString() << std::endl;
  }

  return dataset;
}

arrow::Status
read_sink(std::shared_ptr<cp::ExecPlan> &&plan,
          std::shared_ptr<arrow::Schema> &&schema,
          arrow::AsyncGenerator<std::optional<cp::ExecBatch>> &&sink_gen) {
  std::shared_ptr<arrow::RecordBatchReader> sink_reader =
      cp::MakeGeneratorReader(schema, std::move(sink_gen),
                              cp::default_exec_context()->memory_pool());

  ARROW_RETURN_NOT_OK(plan->StartProducing());
  std::shared_ptr<arrow::Table> result_set;
  ARROW_ASSIGN_OR_RAISE(result_set,
                        arrow::Table::FromRecordBatchReader(sink_reader.get()));
  std::cout << "Results : " << result_set->Slice(0, 1)->ToString() << std::endl;
  plan->StopProducing();

  auto future = plan->finished();
  return future.status();
}

arrow::Status do_taxi(const std::string &dataset_dir,
                      const std::string &out_dir) {
  ARROW_ASSIGN_OR_RAISE(auto file_sys,
                        fs::FileSystemFromUriOrPath(dataset_dir));
  ARROW_ASSIGN_OR_RAISE(auto dataset, get_dataset(dataset_dir));

  auto *registry = cp::default_exec_factory_registry();
  ds::internal::InitializeScanner(registry);
  ds::internal::InitializeDatasetWriter(registry);
  ARROW_ASSIGN_OR_RAISE(auto plan,
                        cp::ExecPlan::Make(cp::default_exec_context()));

  auto options = std::make_shared<ds::ScanOptions>();
  options->projection = cp::project({}, {});
  auto format = std::make_shared<arrow::dataset::ParquetFileFormat>();
  arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen;

  auto status =
      cp::Declaration::Sequence(
          {
              {"scan",
               ds::ScanNodeOptions{
                   dataset,
                   options,
               }},
              {"filter", cp::FilterNodeOptions{cp::greater(

                             cp::field_ref("total_amount"), cp::literal(30))}},
              {"project",
               cp::ProjectNodeOptions{{cp::field_ref("total_amount")}}},
              {"sink", cp::SinkNodeOptions{&sink_gen}},
          })
          .AddToPlan(plan.get());

  auto schema = arrow::schema({
      arrow::field("total_amount", arrow::float64()),
  });

  return read_sink(std::move(plan), std::move(schema), std::move(sink_gen));
}

} // namespace

void test_arrow() { XCHECK(do_taxi(FLAGS_dataset_dir, FLAGS_out_dir).ok()); }
} // namespace bapid
