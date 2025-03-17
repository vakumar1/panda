#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <bitset>
#include <optional>
#include <any>
#include <unordered_set>

#include "absl/strings/str_join.h"
#include "src/model/table.h"

int main() {
    std::vector<std::string> v = {"foo", "bar", "baz"};
    std::string s = absl::StrJoin(v, "-");
    std::cout << "Joined string: " << s << "\n";

    std::unordered_set<HashableRow<int, double, int>> data1;
    for (std::size_t i = 0; i < 5; i++) {
        std::any entry0 = std::any((int) i);
        std::any entry1 = std::any((double) 2 * i);
        std::any entry2 = std::any();
        std::array<std::any, 3> row = {entry0, entry1, entry2};
        data1.insert(HashableRow<int, double, int>(row));
    }
    Table t1 = Table<int, double, int>{
      data1,
      std::bitset<3>("110"),
    };
    print(t1);

    std::unordered_set<HashableRow<int, double, int>> data2;
    for (std::size_t i = 5; i < 10; i++) {
        std::any entry0 = std::any();
        std::any entry1 = std::any((double) 2 * i);
        std::any entry2 = std::any((int) i * 4);
        std::array<std::any, 3> row = {entry0, entry1, entry2};
        data2.insert(HashableRow<int, double, int>(row));
    }
    Table<int, double, int> t2 = Table<int, double, int>{
      data2,
      std::bitset<3>("011"),
    };
    print(t2);

    Table<int, double, int> proj = project(t1, std::bitset<3>("010"));
    // print(proj);
    Dictionary<int, double, int> cons = construction(t2, std::bitset<3>("010"), std::bitset<3>("001"));
    Table<int, double, int> joined = join(proj, cons);
    // print(joined);
    return 0;
}
