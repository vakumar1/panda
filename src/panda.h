#include <array>
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <functional>
#include <memory>
#include <deque>

#include "src/panda_cases.h"
#include "src/panda_utils.h"
#include "src/model/panda.h"
#include "src/model/table.h"
#include "src/model/row.h"

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_subproblem_subnodes(const Subproblem<GlobalSchema...>& subproblem);

template<typename... GlobalSchema>
std::vector<std::pair<Subproblem<GlobalSchema...>, Monotonicity<GlobalSchema...>>> generate_subproblem_leaves(const Subproblem<GlobalSchema...> subproblem);

// function definitions

template<typename... GlobalSchema>
std::unordered_map<Monotonicity<GlobalSchema...>, Table<GlobalSchema...>> generate_ddr_feasible_output(const Subproblem<GlobalSchema...> subproblem) {
    std::vector<std::pair<Subproblem<GlobalSchema...>, Monotonicity<GlobalSchema...>>> leaves = generate_subproblem_leaves(subproblem);
    std::unordered_map<Monotonicity<GlobalSchema...>, Table<GlobalSchema...>> feasible_output;
    for (const auto& leaf : leaves) {
        Subproblem<GlobalSchema...> subproblem = leaf.first;
        Monotonicity<GlobalSchema...> monotonicity = leaf.second;
        Table<GlobalSchema...> sub_table = subproblem.Tn_tables.at(monotonicity).at(0).first;
        if (feasible_output.count(monotonicity) == 0) {
            feasible_output[monotonicity] = sub_table;
        } else {
            inplace_union(feasible_output[monotonicity], sub_table);
        }
    }
    return feasible_output;
}

template<typename... GlobalSchema>
std::vector<std::pair<Subproblem<GlobalSchema...>, Monotonicity<GlobalSchema...>>> generate_subproblem_leaves(const Subproblem<GlobalSchema...> subproblem) {
    std::deque<Subproblem<GlobalSchema...>> curr_problems;
    curr_problems.push_back(subproblem);
    std::vector<std::pair<Subproblem<GlobalSchema...>, Monotonicity<GlobalSchema...>>> leaves;
    std::size_t level = 0;
    while (!curr_problems.empty()) {
        std::size_t curr_size = curr_problems.size();
        for (std::size_t i = 0; i < curr_size; i++) {
            Subproblem<GlobalSchema...> curr_problem = curr_problems.front();
            curr_problems.pop_front();
            std::optional<Monotonicity<GlobalSchema...>> leaf = is_leaf(subproblem, curr_problem);
            if (leaf) {
                leaves.push_back(std::pair<Subproblem<GlobalSchema...>, Monotonicity<GlobalSchema...>>(curr_problem, *leaf));
            } else {
                std::vector<Subproblem<GlobalSchema...>> child_subproblems = generate_subproblem_subnodes(curr_problem);
                curr_problems.insert(curr_problems.end(), std::begin(child_subproblems), std::end(child_subproblems));
            }
        }
        level++;
    }
    return leaves;
}

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_subproblem_subnodes(const Subproblem<GlobalSchema...>& subproblem) {
    std::vector<Subproblem<GlobalSchema...>> subnodes;
    std::vector<Monotonicity<GlobalSchema...>> unconditional_monotonicities;
    for (const auto& [monotonicity, count] : subproblem.D) {
        if (is_unconditional_monotonicity(monotonicity)) {
            unconditional_monotonicities.push_back(monotonicity);
        }
    }
    if (unconditional_monotonicities.size() == 0) {
        throw std::runtime_error("No unconditional monotonicity found");
    }

    for (const auto& unconditional_monotonicity : unconditional_monotonicities) {
        std::optional<Monotonicity<GlobalSchema...>> condition_monotonicity = find_condition_monotonicity(subproblem, unconditional_monotonicity);
        if (condition_monotonicity) {
            subnodes.push_back(generate_condition_subproblem(subproblem, unconditional_monotonicity, *condition_monotonicity));
            return subnodes;
        }

        std::optional<Monotonicity<GlobalSchema...>> split_monotonicity = find_split_monotonicity(subproblem, unconditional_monotonicity);
        if (split_monotonicity) {
            subnodes.push_back(generate_split_subproblem(subproblem, unconditional_monotonicity, *split_monotonicity));
            return subnodes;
        }

        std::optional<Submodularity<GlobalSchema...>> partition_submodularity = find_partition_submodularity(subproblem, unconditional_monotonicity);
        if (partition_submodularity) {
            return generate_partition_subproblems(subproblem, unconditional_monotonicity, *partition_submodularity);
        }
    }

    throw std::runtime_error("No unconiditional monotonicity matched a case");
}
