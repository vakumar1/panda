#pragma once

#include "src/model/panda.h"
#include "src/model/table.h"
#include "src/model/row.h"

template<typename... GlobalSchema>
bool is_leaf(const Subproblem<GlobalSchema...>& original_subproblem, const Subproblem<GlobalSchema...>& subproblem) {
    bool found_output_monotonicity = false;
    for (const auto& [monotonicity, count] : subproblem.D) {
        for (const auto& [output_attributes, count] : original_subproblem.Z) {
            if (monotonicity.attrs_Y == output_attributes && is_unconditional_monotonicity(monotonicity)) {
                found_output_monotonicity = true;
                break;
            }
        }
    }
    return found_output_monotonicity;
}

template<typename... GlobalSchema>
bool is_unconditional_monotonicity(const Monotonicity<GlobalSchema...>& monotonicity) {
    return monotonicity.attrs_X.none();
}


template<typename K, typename... GlobalSchema>
void decrement_count(std::unordered_map<K, unsigned>& map, const K& key) {
    map[key]--;
    if (map[key] == 0) {
        map.erase(key);
    }
}

template<typename K, typename... GlobalSchema>
void increment_count(std::unordered_map<K, unsigned>& map, const K& key) {
    map[key]++;
}

// reset lemma - removes monotonicity and other terms from Shannon inequality while maintaining inequality
template<typename... GlobalSchema>
Subproblem<GlobalSchema...> apply_reset_lemma(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity) {
    
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_ = subproblem.D;
    decrement_count(D_, monotonicity);

    // Case 0: W in Z (base case)
    for (const auto& [output_attrs, count] : subproblem.Z) {
        if (monotonicity.attrs_Y == output_attrs && monotonicity.attrs_X == NULL_ATTR<GlobalSchema...>) {
            // remove Z
            std::unordered_map<OutputAttributes<GlobalSchema...>, unsigned> Z_ = subproblem.Z;
            decrement_count(Z_, output_attrs);
            Subproblem<GlobalSchema...> retry_subproblem = Subproblem(
                Z_,
                D_,
                subproblem.Tn_tables,
                subproblem.Tn_dicts,
                subproblem.M,
                subproblem.S,
                subproblem.global_bound);
            return retry_subproblem;
        }
    }
    
    // Case 1: Y|W in D (inductive case)
    for (const auto& [condition_monotonicity, count] : subproblem.D) {
        if (monotonicity.attrs_Y == condition_monotonicity.attrs_X) {
            // remove Y|W, add YW|0 to D
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_ = subproblem.D;
            Monotonicity<GlobalSchema...> mon_YW = Monotonicity<GlobalSchema...>{
                condition_monotonicity.attrs_X ^ condition_monotonicity.attrs_Y,
                NULL_ATTR<GlobalSchema...>,
            };
            decrement_count(D_, condition_monotonicity);
            increment_count(D_, mon_YW);

            // remove Y|W from dicts
            std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, Constraint>>> Tn_dicts_ = subproblem.Tn_dicts;
            std::pair<Dictionary<GlobalSchema...>, Constraint> Tn_dict_Y_W = Tn_dicts_[condition_monotonicity].back();
            Tn_dicts_[condition_monotonicity].pop_back();

            // retry reset to remove YW|0
            Subproblem<GlobalSchema...> retry_subproblem = Subproblem(
                subproblem.Z,
                D_,
                subproblem.Tn_tables,
                Tn_dicts_,
                subproblem.M,
                subproblem.S,
                subproblem.global_bound);
            return apply_reset_lemma(retry_subproblem, mon_YW);
        }
    }

    // Case 2: W = XY and Y|X in M (inductive case)
    for (const auto& [witness_monotonicity, count] : subproblem.M) {
        if (monotonicity.attrs_Y == (witness_monotonicity.attrs_X ^ witness_monotonicity.attrs_Y)) {

            // remove Y|X from M
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M_ = subproblem.M;
            decrement_count(M_, witness_monotonicity);

            // add X to D
            Monotonicity<GlobalSchema...> mon_X = Monotonicity<GlobalSchema...>{
                witness_monotonicity.attrs_X,
                NULL_ATTR<GlobalSchema...>,
            };
            increment_count(D_, mon_X);

            // retry reset to remove X|0
            Subproblem<GlobalSchema...> retry_subproblem = Subproblem(
                subproblem.Z,
                D_,
                subproblem.Tn_tables,
                subproblem.Tn_dicts,
                M_,
                subproblem.S,
                subproblem.global_bound);
            return apply_reset_lemma(retry_subproblem, mon_X);
        }
    }

    // Case 3: W = XY and Y;Z|X in S (inductive case)
    for (const auto& [witness_submodularity, count] : subproblem.S) {
        if (monotonicity.attrs_Y == (witness_submodularity.attrs_X ^ witness_submodularity.attrs_Y)) {

            // remove Y;Z|X from S
            std::unordered_map<Submodularity<GlobalSchema...>, unsigned> S_ = subproblem.S;
            decrement_count(S_, witness_submodularity);

            // add XYZ to D and add Z|X to M
            Monotonicity<GlobalSchema...> mon_XYZ = Monotonicity<GlobalSchema...>{
                witness_submodularity.attrs_X ^ witness_submodularity.attrs_Y ^ witness_submodularity.attrs_Z,
                NULL_ATTR<GlobalSchema...>,
            };
            increment_count(D_, mon_XYZ);
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M_ = subproblem.M;
            Monotonicity<GlobalSchema...> mon_Z_X = Monotonicity<GlobalSchema...>{
                witness_submodularity.attrs_Z,
                witness_submodularity.attrs_X,
            };
            increment_count(M_, mon_Z_X);

            // retry reset to remove XYZ
            Subproblem<GlobalSchema...> retry_subproblem = Subproblem(
                subproblem.Z,
                D_,
                subproblem.Tn_tables,
                subproblem.Tn_dicts,
                M_,
                S_,
                subproblem.global_bound);
            return apply_reset_lemma(retry_subproblem, mon_XYZ);
        }
    }
    
    throw std::runtime_error("No case matched reset lemma subproblem");
}
