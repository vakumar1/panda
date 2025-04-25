
#include "examples/utils.h"
#include "src/model/table.h"
#include "src/panda.h"

#include <unistd.h>

static std::string spec_dir("/home/vasanth/work/panda/examples/specs");
static std::string spec_file("example1.yaml");
static std::string table_dir("/home/vasanth/work/panda/examples/tables");

int main(void) {
    Subproblem<int, double, std::string, int> init_subproblem = parse_spec<int, double, std::string, int>(spec_dir, spec_file, table_dir);
    std::unordered_map<Monotonicity<int, double, std::string, int>, Table<int, double, std::string, int>> output = generate_ddr_feasible_output<int, double, std::string, int>(init_subproblem);
    std::cout << "==== FEASIBLE DDR OUTPUT ====" << std::endl;
    for (const auto& [mon, table] : output) {
        print(table);
    }
}
