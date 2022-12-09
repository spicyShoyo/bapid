#include "src/arrow.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

namespace bapid {
TEST(ArrowTest, BasicFilter) {
  auto dataset_dir =
      std::filesystem::current_path().string() + "/src/tests/fixtures";
  auto result = BapidTable::describeFsDataset(dataset_dir);
  EXPECT_TRUE(result.hasValue());

  auto table = BapidTable::fromFsDataset(dataset_dir, "taxi");
  EXPECT_TRUE(table.hasValue());

  const auto total_rows = 10;
  const auto min_vals = 30;
  auto query = table.value()
                   ->newSamplesQueryX()
                   .filter(DBL_GT("tip_amount", total_rows))
                   .filter(DBL_GT("tolls_amount", total_rows))
                   .project(DBL_COL("tip_amount"))
                   .project(DBL_COL("tolls_amount"))
                   .project(DBL_COL("total_amount"))
                   .take(total_rows);
  auto result_set = std::move(query).finalize().value().gen().value();
  EXPECT_EQ(result_set->num_columns(), 3);
  EXPECT_EQ(result_set->num_rows(), total_rows);
}
} // namespace bapid
