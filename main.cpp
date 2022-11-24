#include "src/bapid_main.h"
#include <arrow/api.h>
#include <arrow/dataset/file_parquet.h>
#include <arrow/filesystem/filesystem.h>
#include <memory>
#include <parquet/arrow/writer.h>
#include <string_view>

namespace {
constexpr std::string_view kRootPath;
}

namespace fs = arrow::fs;
namespace ds = arrow::dataset;
namespace pq = parquet;

arrow::Result<std::shared_ptr<arrow::Table>> createTable() {
  auto schema = arrow::schema({
      arrow::field("ts", arrow::timestamp(arrow::TimeUnit::SECOND)),
      arrow::field("event", arrow::utf8()),
  });
  std::vector<std::shared_ptr<arrow::Array>> arrays{2};

  arrow::TimestampBuilder ts_builder{arrow::timestamp(arrow::TimeUnit::SECOND),
                                     arrow::default_memory_pool()};
  ARROW_RETURN_NOT_OK(ts_builder.AppendValues({1669320780}));
  ARROW_RETURN_NOT_OK(ts_builder.Finish(&arrays[0]));

  arrow::StringBuilder str_builder{};
  ARROW_RETURN_NOT_OK(str_builder.AppendValues({"ok"}));
  ARROW_RETURN_NOT_OK(str_builder.Finish(&arrays[1]));

  return arrow::Table::Make(schema, arrays);
}

arrow::Status do_read() {
  const auto root_path = std::string{kRootPath};
  ARROW_ASSIGN_OR_RAISE(auto file_sys, fs::FileSystemFromUriOrPath(root_path));

  ARROW_ASSIGN_OR_RAISE(auto table, createTable());
  std::cout << "write " << table->num_rows() << " rows" << std::endl;
  std::cout << table->ToString() << std::endl;

  ARROW_ASSIGN_OR_RAISE(auto output,
                        file_sys->OpenOutputStream(root_path + "a.parquet"));
  ARROW_RETURN_NOT_OK(pq::arrow::WriteTable(
      *table, arrow::default_memory_pool(), output, /*chunk_size=*/2048));

  return arrow::Status::OK();
}

arrow::Status do_write() {
  const auto root_path = std::string{kRootPath};
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
  ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
  ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
  ARROW_ASSIGN_OR_RAISE(auto table, scanner->ToTable());

  std::cout << "Read " << table->num_rows() << " rows" << std::endl;
  std::cout << table->ToString() << std::endl;
  return arrow::Status::OK();
}

int main(int argc, char **argv) {
  XCHECK(do_write().ok());
  XCHECK(do_read().ok());
  return 0;
}
