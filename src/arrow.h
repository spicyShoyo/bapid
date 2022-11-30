#include <arrow/api.h>
#include <arrow/compute/exec/exec_plan.h>
#include <arrow/dataset/file_parquet.h>
#include <arrow/filesystem/filesystem.h>
#include <folly/Expected.h>
#include <memory>
#include <optional>
#include <string>

namespace bapid {

namespace fs = arrow::fs;
namespace ds = arrow::dataset;
namespace pq = parquet;
namespace cp = arrow::compute;

class SamplesQuery {
public:
  static folly::Expected<SamplesQuery, std::string>
  fromDataset(std::shared_ptr<ds::Dataset> dataset);

  SamplesQuery(cp::ExecFactoryRegistry *registry,
               std::shared_ptr<cp::ExecPlan> plan,
               std::shared_ptr<ds::Dataset> dataset);

  class RunnableQuery {
  public:
    folly::Expected<std::shared_ptr<arrow::Table>, std::string> gen() &&;
    RunnableQuery(std::shared_ptr<cp::ExecPlan> plan,
                  std::shared_ptr<arrow::Schema> schema,
                  arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen,
                  std::optional<int> take);

  private:
    std::shared_ptr<cp::ExecPlan> plan_;
    std::shared_ptr<arrow::Schema> schema_;
    arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen_;
    std::optional<int> take_;
  };

  // TODO(liuz): params
  SamplesQuery &filter();
  SamplesQuery &project();
  SamplesQuery &take(int to_take);
  folly::Expected<RunnableQuery, std::string> finalize() &&;

private:
  cp::ExecFactoryRegistry *registry_;
  std::shared_ptr<cp::ExecPlan> plan_;
  std::shared_ptr<ds::Dataset> dataset_;

  std::vector<cp::Expression> filters_{};
  std::vector<cp::Expression> projects_{};
  std::vector<std::shared_ptr<arrow::Field>> result_set_schema_{};
  std::vector<cp::Declaration> decls_{};
  std::optional<int> take_;
};

class BapidTable {
public:
  static folly::Expected<std::unique_ptr<BapidTable>, std::string>
  fromFsDataset(const std::string &dataset_dir, std::string name);

  BapidTable(std::string name, std::shared_ptr<ds::Dataset> dataset);
  SamplesQuery newSamplesQueryX();

private:
  std::string name_;
  std::shared_ptr<ds::Dataset> dataset_;
};

void test_arrow();
} // namespace bapid
