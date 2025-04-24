#include "src/panda_utils.h"
#include "src/model/panda.h"
#include "src/model/table.h"
#include "src/model/row.h"

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
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, Constraint>>> Tn_tables_ = subproblem.Tn_tables;
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, Constraint>>> Tn_dicts_ = subproblem.Tn_dicts;
    std::pair<Table<GlobalSchema...>, Constraint> Tn_table_W = Tn_tables_[monotonicity].back();
    std::pair<Dictionary<GlobalSchema...>, Constraint> Tn_dict_Y_W = Tn_dicts_[condition_monotonicity].back();
    Tn_tables_[monotonicity].pop_back();
    Tn_dicts_[condition_monotonicity].pop_back();
    Constraint N_W = Tn_table_W.second;
    Constraint N_Y_W = Tn_dict_Y_W.second;
    Constraint N_YW = N_W * N_Y_W;

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
        // Case 1.1 - join within bounds
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
        // Case 1.2 - join not within bounds
        // apply reset lemma to remove YW|0
        Subproblem<GlobalSchema...> inter_problem = Subproblem(
            subproblem.Z,
            D_,
            Tn_tables_,
            Tn_dicts_,
            subproblem.M,
            subproblem.S,
            subproblem.global_bound
        );
        return apply_reset_lemma(inter_problem, mon_YW);
    }    
}

// Case 2: split monotonicity
template<typename... GlobalSchema>
std::optional<Monotonicity<GlobalSchema...>> find_split_monotonicity(const Subproblem<GlobalSchema...>& subproblem,
    const Monotonicity<GlobalSchema...>& monotonicity) {
    for (const auto& [split_monotonicity, count] : subproblem.M) {
        if (monotonicity.attrs_Y == (split_monotonicity.attrs_Y ^ split_monotonicity.attrs_X)
            && (split_monotonicity.attrs_Y != NULL_ATTR<GlobalSchema...>)
            && (split_monotonicity.attrs_X != NULL_ATTR<GlobalSchema...>)) {
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
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, Constraint>>> Tn_tables_ = subproblem.Tn_tables;
    std::pair<Table<GlobalSchema...>, Constraint> Tn_table_XY = Tn_tables_[monotonicity].back();
    Tn_tables_[monotonicity].pop_back();
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
        if (monotonicity.attrs_Y == (partition_submodularity.attrs_Y ^ partition_submodularity.attrs_X)
            && (partition_submodularity.attrs_Y != NULL_ATTR<GlobalSchema...>)
            && (partition_submodularity.attrs_X != NULL_ATTR<GlobalSchema...>)) {
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
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, Constraint>>> Tn_tables_ = subproblem.Tn_tables;
    std::pair<Table<GlobalSchema...>, Constraint> Tn_table_XY = Tn_tables_[monotonicity].back();
    Tn_tables_[monotonicity].pop_back();
    std::vector<Table<GlobalSchema...>> Tn_table_XY_partitions = partition(Tn_table_XY.first, partition_submodularity.attrs_X);
    std::vector<Subproblem<GlobalSchema...>> partition_subproblems = {};
    for (const auto& Tn_table_XY_i : Tn_table_XY_partitions) {
        // add X_i to tables (note XY is already removed here)
        std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, Constraint>>> partition_Tn_tables_ = Tn_tables_;
        Table<GlobalSchema...> Tn_table_X_i = project(Tn_table_XY_i, partition_submodularity.attrs_X);
        Constraint N_X_i = Tn_table_X_i.data.size();
        if (partition_Tn_tables_.count(mon_X) == 0) {
            partition_Tn_tables_[mon_X] = {};
        }
        partition_Tn_tables_[mon_X].push_back(std::make_pair(Tn_table_X_i, N_X_i));

        // add YXZ_i to dicts
        std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, Constraint>>> partition_Tn_dicts_ = subproblem.Tn_dicts;
        Dictionary<GlobalSchema...> Tn_dict_Y_X_i = construction(Tn_table_XY_i, partition_submodularity.attrs_X, partition_submodularity.attrs_Y);
        Dictionary<GlobalSchema...> Tn_dict_Y_XZ_i = extension(Tn_dict_Y_X_i, partition_submodularity.attrs_Z);
        Constraint N_Y_XZ_i = degree(Tn_dict_Y_XZ_i);
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
