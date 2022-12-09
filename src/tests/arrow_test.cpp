#include "src/arrow.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

namespace bapid {

namespace {
void EXPECT_ALL_GT(const std::shared_ptr<arrow::Table> &result_set,
                   const std::string &col, double val) {
  auto tip_amount_chunks = result_set->GetColumnByName(col);
  for (auto i = 0; i < tip_amount_chunks->num_chunks(); i++) {
    auto arr = tip_amount_chunks->chunk(i);
    if (arr->length() == 0) {
      continue;
    }

    auto options = cp::ScalarAggregateOptions{};
    auto res = cp::CallFunction("min", {arr}, &options);
    EXPECT_TRUE(res.ok());
    EXPECT_GT(res.ValueOrDie().scalar_as<arrow::DoubleScalar>().value, val);
  }
}

} // namespace

TEST(ArrowTest, BasicFilter) {
  auto dataset_dir =
      std::filesystem::current_path().string() + "/src/tests/fixtures";
  auto result = BapidTable::describeFsDataset(dataset_dir);
  EXPECT_TRUE(result.hasValue());

  auto table = BapidTable::fromFsDataset(dataset_dir, "taxi");
  EXPECT_TRUE(table.hasValue());

  const auto total_rows = 10;
  const auto min_val = 30;
  auto query = table.value()
                   ->newSamplesQueryX()
                   .filter(DBL_GT("tip_amount", min_val))
                   .filter(DBL_GT("tolls_amount", min_val))
                   .project(DBL_COL("tip_amount"))
                   .project(DBL_COL("tolls_amount"))
                   .project(DBL_COL("total_amount"))
                   .take(total_rows);
  auto result_set = std::move(query).finalize().value().gen().value();
  EXPECT_EQ(result_set->num_columns(), 3);
  EXPECT_EQ(result_set->num_rows(), total_rows);

  EXPECT_ALL_GT(result_set, "tip_amount", min_val);
  EXPECT_ALL_GT(result_set, "tolls_amount", min_val);
  EXPECT_ALL_GT(result_set, "total_amount", 2 * min_val);
}
} // namespace bapid
