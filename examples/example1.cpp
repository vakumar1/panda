
#include "examples/utils.h"
#include "src/model/table.h"
#include "src/panda.h"

#include <unistd.h>

// TODO: add example-specific metadata to spec file -> load/parse spec to generate subproblem:
// - map from global schema index to column name
// - list of output queries (i.e. Z attributes)
// - degree constraints (TODO: add general table/dict loading)
// - monotonicities + counts
// - submodularities + counts
// - w parameters for computing global bound

static std::string table_dir("/home/vasanth/work/panda/examples/tables");
static std::string t1_file("example1_t1.csv");
static std::string t2_file("example1_t2.csv");
static std::string t3_file("example1_t3.csv");

static attr_type<int, double, std::string, int> A = std::bitset<4>("0001");
static attr_type<int, double, std::string, int> B = std::bitset<4>("0010");
static attr_type<int, double, std::string, int> C = std::bitset<4>("0100");
static attr_type<int, double, std::string, int> D = std::bitset<4>("1000");


// 0011
Table<int, double, std::string, int> load_t1() {
    std::cout << "Loading t1" << std::endl;
    std::vector<std::vector<std::string>> rows = load_csv(table_dir + "/" + t1_file);
    std::function<HashableRow<int, double, std::string, int>(std::vector<std::string>)> parser = [](std::vector<std::string> raw_row){
        if (raw_row.size() != 2) {
            throw std::runtime_error("Invalid row in t1 does not match schema.");
        }
        std::array<std::any, 4> parsed_row = {
            std::any(std::stoi(raw_row.at(0))),
            std::any(std::stod(raw_row.at(1))),
            std::any(),
            std::any()
        };
        return HashableRow<int, double, std::string, int>(parsed_row);
    };
    return parse_table<int, double, std::string, int>(
        rows, 
        parser,
        A ^ B
    );
}

// 0110
Table<int, double, std::string, int> load_t2() {
    std::cout << "Loading t2" << std::endl;
    std::vector<std::vector<std::string>> rows = load_csv(table_dir + "/" + t2_file);
    std::function<HashableRow<int, double, std::string, int>(std::vector<std::string>)> parser = [](std::vector<std::string> raw_row){
        if (raw_row.size() != 2) {
            throw std::runtime_error("Invalid row in t2 does not match schema.");
        }
        std::array<std::any, 4> parsed_row = {
            std::any(),
            std::any(std::stod(raw_row.at(0))),
            std::any(raw_row.at(1)),
            std::any()
        };
        return HashableRow<int, double, std::string, int>(parsed_row);
    };
    return parse_table(
        rows, 
        parser,
        B ^ C
    );
}

// 1100
Table<int, double, std::string, int> load_t3() {
    std::cout << "Loading t3" << std::endl;
    std::vector<std::vector<std::string>> rows = load_csv(table_dir + "/" + t3_file);
    std::function<HashableRow<int, double, std::string, int>(std::vector<std::string>)> parser = [](std::vector<std::string> raw_row){
        if (raw_row.size() != 2) {
            throw std::runtime_error("Invalid row in t3 does not match schema.");
        }
        std::array<std::any, 4> parsed_row = {
            std::any(),
            std::any(),
            std::any(raw_row.at(0)),
            std::any(std::stoi(raw_row.at(1)))
        };
        return HashableRow<int, double, std::string, int>(parsed_row);
    };
    return parse_table(
        rows, 
        parser,
        C ^ D
    );
}

int main(void) {
    Table<int, double, std::string, int> t1 = load_t1();
    print(t1);
    std::cout << std::endl;
    Table<int, double, std::string, int> t2 = load_t2();
    print(t2);
    std::cout << std::endl;
    Table<int, double, std::string, int> t3 = load_t3();
    print(t3);
    std::cout << std::endl;

    // TODO: load from spec
    const std::unordered_map<OutputAttributes<int, double, std::string, int>, unsigned> Z = {
        {A ^ B ^ C, 1},
        {B ^ C ^ D, 1}
    };
    Monotonicity<int, double, std::string, int> m1 = {
        t1.attributes,
        NULL_ATTR<int, double, std::string, int>
    };
    Monotonicity<int, double, std::string, int> m2 = {
        t2.attributes,
        NULL_ATTR<int, double, std::string, int>
    };
    Monotonicity<int, double, std::string, int> m3 = {
        t3.attributes,
        NULL_ATTR<int, double, std::string, int>
    };
    const std::unordered_map<Monotonicity<int, double, std::string, int>, unsigned> D_ = {
        {m1, 1},
        {m2, 1},
        {m3, 1}
    };
    const std::unordered_map<Monotonicity<int, double, std::string, int>, std::vector<std::pair<Table<int, double, std::string, int>, Constraint>>> Tn_tables = {
        {m1, {std::make_pair(t1, t1.data.size())}},
        {m2, {std::make_pair(t2, t2.data.size())}},
        {m3, {std::make_pair(t3, t3.data.size())}}
    };
    const std::unordered_map<Monotonicity<int, double, std::string, int>, std::vector<std::pair<Dictionary<int, double, std::string, int>, Constraint>>> Tn_dicts;
    const std::unordered_map<Monotonicity<int, double, std::string, int>, unsigned> M;
    Submodularity<int, double, std::string, int> s1 = {
        A, C, B
    };
    Submodularity<int, double, std::string, int> s2 = {
        B, C ^ D, NULL_ATTR<int, double, std::string, int>
    };
    const std::unordered_map<Submodularity<int, double, std::string, int>, unsigned> S = {
        {s1, 1},
        {s2, 1}
    };
    const long double global_bound = std::pow((long double) t1.data.size(), (long double) 0.5)
        * std::pow((long double) t2.data.size(), (long double) 0.5)
        * std::pow((long double) t3.data.size(), (long double) 0.5);
    Subproblem<int, double, std::string, int> init_subproblem(
        Z,
        D_,
        Tn_tables,
        Tn_dicts,
        M,
        S,
        global_bound
    );

    std::unordered_map<Monotonicity<int, double, std::string, int>, Table<int, double, std::string, int>> output = generate_ddr_feasible_output<int, double, std::string, int>(init_subproblem);
    std::cout << "==== FEASIBLE DDR OUTPUT ====" << std::endl;
    for (const auto& [mon, table] : output) {
        print(table);
    }
}