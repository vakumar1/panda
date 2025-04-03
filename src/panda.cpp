#include "panda.h"

// function declarations

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_subproblem_subnodes(const Subproblem<GlobalSchema...>& subproblem);

template<typename... GlobalSchema>
std::optional<Monotonicity<GlobalSchema...>> find_condition_monotonicity(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity);

template<typename... GlobalSchema>
Subproblem<GlobalSchema...> generate_condition_subproblem(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity,
    const Monotonicity<GlobalSchema...>& condition_monotonicity);

template<typename... GlobalSchema>
std::optional<Monotonicity<GlobalSchema...>> find_split_monotonicity(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity);

template<typename... GlobalSchema>
Subproblem<GlobalSchema...> generate_split_subproblem(const Subproblem<GlobalSchema...>& subproblem, 
    const Monotonicity<GlobalSchema...>& monotonicity,
    const Monotonicity<GlobalSchema...>& split_monotonicity);

template<typename... GlobalSchema>
std::optional<Submodularity<GlobalSchema...>> find_partition_submodularity(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity);

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_partition_subproblems(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity,
    const Submodularity<GlobalSchema...>& partition_submodularity);

template<typename... GlobalSchema>
bool is_leaf(const Subproblem<GlobalSchema...>& original_subproblem, const Subproblem<GlobalSchema...>& subproblem);

template<typename... GlobalSchema>
bool is_unconditional_monotonicity(const Monotonicity<GlobalSchema...>& monotonicity);

template<typename K, typename... GlobalSchema>
void decrement_count(std::unordered_map<K, unsigned>& map, const K& key);

template<typename K, typename... GlobalSchema>
void increment_count(std::unordered_map<K, unsigned>& map, const K& key);

template<typename... GlobalSchema>
Subproblem<GlobalSchema...> apply_reset_lemma(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity);

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

// subproblem helpers

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
        subnodes.push_back(generate_partition_subproblem(subproblem, unconditional_monotonicity, *partition_submodularity));
        return subnodes;
    }

    throw std::runtime_error("No case matched subproblem");
}

// Case 1: condition monotonicity
template<typename... GlobalSchema>
std::optional<Monotonicity<GlobalSchema...>> find_condition_monotonicity(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity) {
    for (const auto& [condition_monotonicity, count] : subproblem.D) {
        if (monotonicity.attrs_Y == condition_monotonicity.attrs_X) {
            return std::optional<Monotonicity<GlobalSchema...>>(condition_monotonicity);
        }
    }
    return std::nullopt;
}

template<typename... GlobalSchema>
Subproblem<GlobalSchema...> generate_condition_subproblem(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity,
    const Monotonicity<GlobalSchema...>& condition_monotonicity) {

    // remove W|0 from tables and remove Y|W from dicts 
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, unsigned>>> Tn_tables_ = subproblem.Tn_tables;
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, unsigned>>> Tn_dicts_ = subproblem.Tn_dicts;
    std::pair<Table<GlobalSchema...>, unsigned> Tn_table_W = Tn_tables_[monotonicity].front();
    std::pair<Dictionary<GlobalSchema...>, unsigned> Tn_dict_Y_W = Tn_dicts_[condition_monotonicity].front();
    Tn_tables_[monotonicity].pop_front();
    Tn_dicts_[condition_monotonicity].pop_front();
    unsigned N_W = Tn_table_W.second;
    unsigned N_Y_W = Tn_dict_Y_W.second;
    unsigned N_YW = N_W * N_Y_W;

    // remove W|0, Y|W from D and add YW|0 to D
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_ = subproblem.D;
    Monotonicity<GlobalSchema...> mon_YW = Monotonicity<GlobalSchema...>{
        condition_monotonicity.attrs_X ^ condition_monotonicity.attrs_Y,
        NULL_ATTR<GlobalSchema...>,
    };
    decrement_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, monotonicity);
    decrement_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, condition_monotonicity);
    increment_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, mon_YW);

    if (N_YW <= subproblem.global_bound) {
        // add join YW|0 to tables
        Table<GlobalSchema...> Tn_table_YW = join(Tn_table_W.first, Tn_dict_Y_W.first);
        Tn_tables_[mon_YW].push_back(std::make_pair(Tn_table_YW, N_YW));

        return Subproblem(
            subproblem.Z,
            D_,
            Tn_tables_,
            Tn_dicts_,
            subproblem.M,
            subproblem.S,
            subproblem.global_bound
        );
    } else {
        // apply reset lemma to remove YW|0
        return apply_reset_lemma(mon_YW);
    }    
}

// Case 2: split monotonicity
template<typename... GlobalSchema>
std::optional<Monotonicity<GlobalSchema...>> find_split_monotonicity(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity) {
    for (const auto& [split_monotonicity, count] : subproblem.M) {
        if (monotonicity.attrs_Y == split_monotonicity.attrs_Y ^ split_monotonicity.attrs_X) {
            return std::optional<Monotonicity<GlobalSchema...>>(split_monotonicity);
        }
    }
    return std::nullopt;
}

template<typename... GlobalSchema>
Subproblem<GlobalSchema...> generate_split_subproblem(const Subproblem<GlobalSchema...>& subproblem, 
    const Monotonicity<GlobalSchema...>& monotonicity,
    const Monotonicity<GlobalSchema...>& split_monotonicity) {

    // remove Y|X from M
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M_ = subproblem.M;
    decrement_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(M_, split_monotonicity);

    // remove XY|0 and add X|0 to D
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_ = subproblem.D;
    Monotonicity<GlobalSchema...> mon_X = Monotonicity<GlobalSchema...>{
        split_monotonicity.attrs_X,
        NULL_ATTR<GlobalSchema...>,
    };
    increment_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, mon_X);
    decrement_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, monotonicity);

    // remove XY and add X to tables
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, unsigned>>> Tn_tables_ = subproblem.Tn_tables;
    std::pair<Table<GlobalSchema...>, unsigned> Tn_table_XY = Tn_tables_[monotonicity].front();
    Tn_tables_[monotonicity].pop_front();
    Table<GlobalSchema...> Tn_table_X = project(Tn_table_XY.first, split_monotonicity.attrs_X);
    if (Tn_tables_.count(mon_X) == 0) {
        Tn_tables_[mon_X] = {};
    }
    Tn_tables_[mon_X].push_back(std::make_pair(Tn_table_X, Tn_table_XY.second));

    return Subproblem(
        subproblem.Z,
        D_,
        Tn_tables_,
        subproblem.Tn_dicts,
        M_,
        subproblem.S,
        subproblem.global_bound
    );
}

// Case 3: partition submodularity
template<typename... GlobalSchema>
std::optional<Submodularity<GlobalSchema...>> find_partition_submodularity(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity) {
    for (const auto& [partition_submodularity, count] : subproblem.S) {
        if (monotonicity.attrs_Y == partition_submodularity.attrs_Y ^ partition_submodularity.attrs_X) {
            return std::optional<Submodularity<GlobalSchema...>>(partition_submodularity);
        }
    }
    return std::nullopt;
}

template<typename... GlobalSchema>
std::vector<Subproblem<GlobalSchema...>> generate_partition_subproblems(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity,
    const Submodularity<GlobalSchema...>& partition_submodularity) {

    // remove XY|0 and add X|0, Y|XZ to D
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_ = subproblem.D;
    Monotonicity<GlobalSchema...> mon_X = Monotonicity<GlobalSchema...>{
        partition_submodularity.attrs_X,
        NULL_ATTR<GlobalSchema...>,
    };
    Monotonicity<GlobalSchema...> mon_YXZ = Monotonicity<GlobalSchema...>{
        partition_submodularity.attrs_Y,
        partition_submodularity.attrs_X ^ partition_submodularity.attrs_Z,
    };
    increment_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, mon_X);
    increment_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, mon_YXZ);
    decrement_count<Monotonicity<GlobalSchema...>, GlobalSchema...>(D_, monotonicity);

    // remove Y;Z|X from S
    std::unordered_map<Submodularity<GlobalSchema...>, unsigned> S_ = subproblem.S;
    decrement_count<Submodularity<GlobalSchema...>, GlobalSchema...>(S_, partition_submodularity);

    // remove XY and add partitions X_i, YXZ_i to tables
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, unsigned>>> Tn_tables_ = subproblem.Tn_tables;
    std::pair<Table<GlobalSchema...>, unsigned> Tn_table_XY = Tn_tables_[monotonicity].front();
    Tn_tables_[monotonicity].pop_front();
    std::vector<Table<GlobalSchema...>> Tn_table_XY_partitions = partition(Tn_table_XY, partition_submodularity.attrs_X);
    std::vector<Subproblem<GlobalSchema...>> partition_subproblems = {};
    for (const auto& Tn_table_XY_i : Tn_table_XY_partitions) {
        // add X_i to tables (note XY is already removed here)
        std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, unsigned>>> partition_Tn_tables_ = Tn_tables_;
        Table<GlobalSchema...> Tn_table_X_i = project(Tn_table_XY_i, partition_submodularity.attrs_X);
        unsigned N_X_i = Tn_table_X_i.data.size();
        if (partition_Tn_tables_.count(mon_X) == 0) {
            partition_Tn_tables_[mon_X] = {};
        }
        partition_Tn_tables_[mon_X].push_back(std::make_pair(Tn_table_X_i, N_X_i));

        // add YXZ_i to dicts
        std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, unsigned>>> partition_Tn_dicts_ = subproblem.Tn_dicts;
        Dictionary<GlobalSchema...> Tn_dict_Y_X_i = construction(Tn_table_XY_i, partition_submodularity.attrs_X, partition_submodularity.attrs_Y);
        Dictionary<GlobalSchema...> Tn_dict_Y_XZ_i = extension(Tn_dict_Y_X_i, partition_submodularity.attrs_Z);
        unsigned N_Y_XZ_i = degree(Tn_dict_Y_XZ_i);
        if (partition_Tn_dicts_.count(mon_YXZ) == 0) {
            partition_Tn_dicts_[mon_YXZ] = {};
        }
        partition_Tn_dicts_[mon_YXZ].push_back(std::make_pair(Tn_dict_Y_XZ_i, N_Y_XZ_i));

        Subproblem<GlobalSchema...> partition_subproblem = Subproblem(
            subproblem.Z,
            D_,
            partition_Tn_tables_,
            partition_Tn_dicts_,
            subproblem.M,
            S_,
            subproblem.global_bound
        );
        partition_subproblems.push_back(partition_subproblem);
    }
    return partition_subproblems;
}

// utils

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
    return monotonicity.attrs_X.empty();
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
    for (const auto& [output_monotonicity, count] : subproblem.Z) {
        if (monotonicity.attrs_Y == output_monotonicity.attrs_Y && monotonicity.attrs_X == output_monotonicity.attrs_X) {
            // remove Z
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> Z_ = subproblem.Z;
            decrement_count(Z_, monotonicity);
            return Subproblem(
                Z_,
                D_,
                subproblem.Tn_tables,
                subproblem.Tn_dicts,
                subproblem.M,
                subproblem.S,
                subproblem.global_bound);
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
            std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, unsigned>>> Tn_dicts_ = subproblem.Tn_dicts;
            std::pair<Dictionary<GlobalSchema...>, unsigned> Tn_dict_Y_W = Tn_dicts_[condition_monotonicity].front();
            Tn_dicts_[condition_monotonicity].pop_front();

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
        if (monotonicity.attrs_Y == witness_monotonicity.attrs_X ^ witness_monotonicity.attrs_Y) {

            // remove Y|X from M
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M_ = subproblem.M;
            decrement_count(M_, witness_monotonicity);

            // add X to D
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_ = subproblem.D;
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
        if (monotonicity.attrs_Y == witness_submodularity.attrs_X ^ witness_submodularity.attrs_Y) {

            // remove Y;Z|X from S
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> S_ = subproblem.S;
            decrement_count(S_, witness_submodularity);

            // add XYZ to D and add Z|X to M
            std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_ = subproblem.D;
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
    
}
