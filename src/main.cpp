#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include "absl/strings/str_join.h"
#include "src/model/table.h"

int main() {
  std::vector<std::string> v = {"foo", "bar", "baz"};
  std::string s = absl::StrJoin(v, "-");

  std::cout << "Joined string: " << s << "\n";

  std::vector<std::tuple<int, double>> data1 = {};
  for (int i = 0; i < 10; i++) {
    data1.push_back({i, (double) 2 * i});
  }
  std::vector<std::tuple<double, int>> data2 = {};
  for (int i = 0; i < 20; i++) {
    data2.push_back({(double) i, -i});
  }
  auto table1 = Table<std::tuple<int, double>> {
    data1,
    {0, 1}
  };
  auto table2 = Table<std::tuple<double, int>> {
    data2,
    {1, 2}
  };
  std::array<size_t, 1> proj_indices = {1};
  auto proj = project(table1, proj_indices);
  std::array<size_t, 1> cons_indices_1 = {1};
  std::array<size_t, 1> cons_indices_2 = {2};
  auto cons = construction(table2, cons_indices_1, cons_indices_2);
  auto joined = join<double, int>(proj, cons);

  return 0;
}
