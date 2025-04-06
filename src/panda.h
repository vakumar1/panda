#include <array>
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <functional>
#include <memory>

#include "src/panda_cases.h"
#include "src/panda_utils.h"
#include "src/model/panda.h"
#include "src/model/table.h"
#include "src/model/row.h"

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_subproblem_subnodes(const Subproblem<GlobalSchema...>& subproblem);

// function definitions

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_subproblem_leaves(const Subproblem<GlobalSchema...> subproblem) {
    std::vector<Subproblem<GlobalSchema...>> curr_problems;
    curr_problems.push_back(subproblem);
    std::vector<Subproblem<GlobalSchema...>> leaves;
    while (!curr_problems.empty()) {
        Subproblem<GlobalSchema...>& curr_problem = curr_problems.back();
        curr_problems.pop_back();
        if (is_leaf(curr_problem)) {
            leaves.push_back(curr_problem);
        } else {
            curr_problems.push_back(generate_subproblem_subnodes(curr_problem));
        }
    }
    return leaves;
}

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_subproblem_subnodes(const Subproblem<GlobalSchema...>& subproblem) {
    std::vector<Subproblem<GlobalSchema...>> subnodes;
    Monotonicity<GlobalSchema...> unconditional_monotonicity;
    bool found_unconditional_monotonicity = false;
    for (const auto& [monotonicity, count] : subproblem.D) {
        if (is_unconditional_monotonicity(monotonicity)) {
            unconditional_monotonicity = monotonicity;
            found_unconditional_monotonicity = true;
            break;
        }
    }
    if (!found_unconditional_monotonicity) {
        throw std::runtime_error("No unconditional monotonicity found");
    }

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

    throw std::runtime_error("No case matched subproblem");
}
