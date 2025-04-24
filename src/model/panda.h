#pragma once

#include <fmt/core.h>

#include "src/utils.h"
#include "src/model/table.h"
#include "src/model/row.h"

using Constraint = long double;

template<typename... GlobalSchema>
using OutputAttributes = attr_type<GlobalSchema...>;

template<typename... GlobalSchema>
struct Monotonicity {
    attr_type<GlobalSchema...> attrs_Y;
    attr_type<GlobalSchema...> attrs_X;

    bool operator==(const Monotonicity& other) const {
        return attrs_Y == other.attrs_Y && attrs_X == other.attrs_X;
    }

    std::string to_string() const {
        return fmt::format("{} | {}", attrs_Y.to_string(), attrs_X.to_string());
    }
};

template<typename... GlobalSchema>
struct Submodularity {
    attr_type<GlobalSchema...> attrs_Y;
    attr_type<GlobalSchema...> attrs_Z;
    attr_type<GlobalSchema...> attrs_X;

    bool operator==(const Submodularity& other) const {
        return attrs_Y == other.attrs_Y && attrs_Z == other.attrs_Z && attrs_X == other.attrs_X;
    }

    std::string to_string() const {
        return fmt::format("{} ; {} | {}", attrs_Y.to_string(), attrs_Z.to_string(), attrs_X.to_string());
    }
};

template<typename... GlobalSchema>
struct Subproblem {
    Subproblem(
        std::unordered_map<OutputAttributes<GlobalSchema...>, unsigned> Z_,
        std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_,
        std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, Constraint>>> Tn_tables_,
        std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, Constraint>>> Tn_dicts_,
        std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M_,
        std::unordered_map<Submodularity<GlobalSchema...>, unsigned> S_,
        long double global_bound_
    ) : Z(Z_), D(D_), Tn_tables(Tn_tables_), Tn_dicts(Tn_dicts_), M(M_), S(S_), global_bound(global_bound_) {
        verify_state();
    }

    std::unordered_map<OutputAttributes<GlobalSchema...>, unsigned> Z;
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D;
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, Constraint>>> Tn_tables;
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, Constraint>>> Tn_dicts;
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M;
    std::unordered_map<Submodularity<GlobalSchema...>, unsigned> S;
    long double global_bound;
    
    void verify_state() const {
        // TODO:
        // - verify each table condition is null set
        // - verify count in D matches each T count
        // - (optional) verify Shannon equality
    }
};

template<typename... GlobalSchema>
struct std::hash<Monotonicity<GlobalSchema...>> {
    std::size_t operator()(const Monotonicity<GlobalSchema...>& m) const {
        std::size_t seed = 0;
        std::hash<attr_type<GlobalSchema...>> hasher;
        seed ^= hasher(m.attrs_Y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hasher(m.attrs_X) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

template<typename... GlobalSchema>
struct std::hash<Submodularity<GlobalSchema...>> {
    std::size_t operator()(const Submodularity<GlobalSchema...>& s) const {
        std::size_t seed = 0;
        std::hash<attr_type<GlobalSchema...>> hasher;
        seed ^= hasher(s.attrs_Y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hasher(s.attrs_Z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hasher(s.attrs_X) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

template<typename... GlobalSchema>
void print(const Subproblem<GlobalSchema...> subproblem) {
    std::cout << "Subproblem: -----" << std::endl;
    
    std::cout << TAB << "Z" << std::endl;
    for (const auto& [output_attrs, count] : subproblem.Z) {
        std::cout << TAB << TAB << output_attrs.to_string() << TAB << count << std::endl;
    }
    
    std::cout << TAB << "D" << std::endl;
    for (const auto& [mon, count] : subproblem.D) {
        std::cout << TAB << TAB << mon.to_string() << TAB << count << std::endl;
    }

    std::cout << TAB << "Tn_tables" << std::endl;
    for (const auto&[mon, tables] : subproblem.Tn_tables) {
        std::cout << TAB << TAB << mon.to_string() << TAB << tables.size() << std::endl;
    }

    std::cout << TAB << "Tn_dicts" << std::endl;
    for (const auto&[mon, dicts] : subproblem.Tn_dicts) {
        std::cout << TAB << TAB << mon.to_string() << TAB << dicts.size() << std::endl;
    }

    std::cout << TAB << "M" << std::endl;
    for (const auto& [mon, count] : subproblem.M) {
        std::cout << TAB << TAB << mon.to_string() << TAB << count << std::endl;
    }

    std::cout << TAB << "S" << std::endl;
    for (const auto& [sub, count] : subproblem.S) {
        std::cout << TAB << TAB << sub.to_string() << TAB << count << std::endl;
    }
    std::cout << "-----" << std::endl;
}