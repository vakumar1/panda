#include <array>
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <functional>
#include <memory>

#include "src/model/table.h"
#include "src/model/row.h"

template<typename... GlobalSchema>
using OutputAttributes = attr_type<GlobalSchema...>;

template<typename... GlobalSchema>
struct Monotonicity {
    attr_type<GlobalSchema...> attrs_Y;
    attr_type<GlobalSchema...> attrs_X;
};

template<typename... GlobalSchema>
struct Submodularity {
    attr_type<GlobalSchema...> attrs_Y;
    attr_type<GlobalSchema...> attrs_Z;
    attr_type<GlobalSchema...> attrs_X;
};

template<typename... GlobalSchema>
struct Subproblem {
    Subproblem(
        const std::unordered_map<OutputAttributes<GlobalSchema...>, unsigned> Z_,
        const std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D_,
        const std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, unsigned>>> Tn_tables_,
        const std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, unsigned>>> Tn_dicts_,
        const std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M_,
        const std::unordered_map<Submodularity<GlobalSchema...>, unsigned> S_,
        const long double global_bound_
    ) : Z(Z_), D(D_), Tn_tables(Tn_tables_), Tn_dicts(Tn_dicts_), M(M_), S(S_), global_bound(global_bound_) {
        verify_state();
    }

    const std::unordered_map<OutputAttributes<GlobalSchema...>, unsigned> Z;
    const std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D;
    const std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, unsigned>>> Tn_tables;
    const std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, unsigned>>> Tn_dicts;
    const std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M;
    const std::unordered_map<Submodularity<GlobalSchema...>, unsigned> S;
    const long double global_bound;
    
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
std::vector<Subproblem<GlobalSchema...>> generate_subproblem_leaves(const Subproblem<GlobalSchema...> subproblem);
