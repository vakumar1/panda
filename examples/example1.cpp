
#include "examples/utils.h"
#include "src/model/table.h"
#include "src/panda.h"

#include <getopt.h>

static std::string spec_file("example1.yaml");

int main(int argc, char* argv[]) {

    const struct option long_options[] = {
        {"spec_dir", required_argument, nullptr, 's'},
        {"tables_dir", required_argument, nullptr, 't'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int long_index = 0;
    std::string spec_dir;
    std::string table_dir;
    while ((opt = getopt_long(argc, argv, "s:t:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 's':
                spec_dir = optarg;
                break;
            case 't':
                table_dir = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [--spec_dir <spec directory>] [--tables_dir <tables directory>]" << std::endl;
                return 1;
        }
    }
    if (spec_dir.empty() || table_dir.empty()) {
        std::string err_msg = "Provide spec dir and tables dir";
        throw std::runtime_error(err_msg);
    }

    Subproblem<int, double, std::string, int> init_subproblem = parse_spec<int, double, std::string, int>(spec_dir, spec_file, table_dir);
    std::unordered_map<Monotonicity<int, double, std::string, int>, Table<int, double, std::string, int>> output = generate_ddr_feasible_output<int, double, std::string, int>(init_subproblem);
    std::cout << "==== FEASIBLE DDR OUTPUT ====" << std::endl;
    for (const auto& [mon, table] : output) {
        print(table);
    }
}
