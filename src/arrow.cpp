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
              {"write",
               ds::WriteNodeOptions{
                   {.file_write_options = format->DefaultWriteOptions(),
                    .filesystem = file_sys,
                    .existing_data_behavior =
                        ds::ExistingDataBehavior::kOverwriteOrIgnore,
                    .partitioning = std::make_shared<ds::HivePartitioning>(
                        arrow::schema({})),
                    .basename_template = "part{i}.parquet",
                    .base_dir = out_dir}}},
          })
          .AddToPlan(plan.get());

  XLOG(INFO) << status.status();
  ARROW_RETURN_NOT_OK(status);
  ARROW_RETURN_NOT_OK(plan->StartProducing());
  auto fut = plan->finished();
  XLOG(INFO) << fut.status();
  ARROW_RETURN_NOT_OK(fut.status());
  fut.Wait();

  return arrow::Status::OK();
}

arrow::Status do_peek(const std::string &out_dir) {
  ARROW_ASSIGN_OR_RAISE(auto dataset, get_dataset(out_dir));
  ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
  auto maybe_scanner = scan_builder->Finish();
  std::cerr << maybe_scanner.status() << std::endl;
  ARROW_ASSIGN_OR_RAISE(auto scanner, maybe_scanner);
  ARROW_ASSIGN_OR_RAISE(auto table, scanner->ToTable());

  std::cout << table->Slice(0, 1)->ToString() << std::endl;
  std::cout << "Read " << table->num_rows() << " rows" << std::endl;
  for (const auto &col : table->ColumnNames()) {
    std::cout << "Col: " << col << std::endl;
  }
  return arrow::Status::OK();
}
} // namespace

void test_arrow() {
  XCHECK(do_taxi(FLAGS_dataset_dir, FLAGS_out_dir).ok());
  XCHECK(do_peek(FLAGS_out_dir).ok());
}
} // namespace bapid
