#include "if/bapid.grpc.pb.h"
#include "if/bapid.pb.h"
#include <arrow/api.h>
#include <arrow/compute/exec/exec_plan.h>
#include <arrow/dataset/file_parquet.h>
#include <arrow/filesystem/filesystem.h>
#include <folly/Expected.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

namespace bapid {

namespace fs = arrow::fs;
namespace ds = arrow::dataset;
namespace pq = parquet;
namespace cp = arrow::compute;

bapidrpc::Col DBL_COL(std::string name);
bapidrpc::Filter DBL_GT(std::string name, double val);

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

  SamplesQuery &filter(const bapidrpc::Filter &filter);
  SamplesQuery &project(const bapidrpc::Col &col);
  SamplesQuery &take(int to_take);
  folly::Expected<RunnableQuery, std::string> finalize() &&;

private:
  cp::ExecFactoryRegistry *registry_;
  std::shared_ptr<cp::ExecPlan> plan_;
  std::shared_ptr<ds::Dataset> dataset_;

  std::vector<cp::Expression> filters_{};
  std::unordered_set<std::string> fields_{};
  std::vector<cp::Expression> projects_{};
  std::vector<std::shared_ptr<arrow::Field>> result_set_schema_{};
  std::vector<cp::Declaration> decls_{};
  std::optional<int> take_;
};

class BapidTable {
public:
  static folly::Expected<folly::Unit, std::string>
  describeFsDataset(const std::string &root_path);

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
