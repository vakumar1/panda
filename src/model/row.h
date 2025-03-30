#include <array>
#include <any>
#include <algorithm>
#include <bitset>
#include <sstream>
#include <string>

template<typename... GlobalSchema>
using attr_type = typename std::bitset<sizeof...(GlobalSchema)>;

template<std::size_t index, typename... GlobalSchema>
using elem_type = typename std::tuple_element_t<index, std::tuple<GlobalSchema...>>;

template<std::size_t index, typename... GlobalSchema>
elem_type<index, GlobalSchema...> safe_get(const std::array<std::any, sizeof...(GlobalSchema)>& data_) {
    try {
        return std::any_cast<elem_type<index, GlobalSchema...>>(data_[index]);
    } catch (const std::bad_any_cast& e) {
        std::string error_msg = "Bad any cast: ";
        error_msg += "index=" + std::to_string(index) + " ";
        error_msg += "expected_type=" + std::string(typeid(elem_type<index, GlobalSchema...>).name()) + " ";
        error_msg += "actual_type=" + std::string(data_[index].type().name());
        throw std::runtime_error(error_msg);
    }
}

template<typename... GlobalSchema>
struct HashableRow {
    static constexpr std::size_t N = sizeof...(GlobalSchema);

    // TODO: make data reference
    std::array<std::any, N> data;
    std::size_t hash;

    HashableRow(const std::array<std::any, N>& data_) {
        data = data_;
        hash = cache_hash(data_);
    }
    
    // == operator
    template<size_t index>
    bool all_eq_idx(const HashableRow& other) const {
        bool this_has_value = data[index].has_value();
        bool other_has_value = other.data[index].has_value();
        if (this_has_value != other_has_value) {
            return false;
        }
        if (this_has_value && other_has_value) {
            if (safe_get<index, GlobalSchema...>(data) != safe_get<index, GlobalSchema...>(other.data)) {
                return false;
            }
        }
        return true;
    }
    
    template<std::size_t... Indices>
    bool all_eq_pack(const HashableRow& other, std::index_sequence<Indices...>) const {
        std::array<bool, N> eq_arr = {all_eq_idx<Indices>(other)...};
        return std::all_of(eq_arr.begin(), eq_arr.end(), [](bool b){ return b; });
    }
    
    bool operator==(const HashableRow& other) const {
        return all_eq_pack(other, std::make_index_sequence<N>{});
    }

    // hash
    template<size_t index>
    std::size_t hash_idx(const std::array<std::any, N>& data_) const {
        if (data[index].has_value()) {
            return std::hash<elem_type<index, GlobalSchema...>>{}(safe_get<index, GlobalSchema...>(data_));
        } else {
            return 0;
        }
    }
    
    template<std::size_t... Indices>
    std::size_t hash_pack(const std::array<std::any, N>& data_, std::index_sequence<Indices...>) const {
        std::array<std::size_t, N> seeds = {hash_idx<Indices>(data_)...};
        std::size_t seed = seeds.size();
        for (std::size_t i = 0; i < seeds.size(); i++) {
            seed ^= seeds[i] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    std::size_t cache_hash(const std::array<std::any, N>& data_) const {
        return hash_pack(data_, std::make_index_sequence<N>{});
    }
};

template<typename... GlobalSchema>
struct std::hash<HashableRow<GlobalSchema...>> {
    std::size_t operator()(const HashableRow<GlobalSchema...>& row) const {
        return row.hash;
    }
};

// row mask
template<typename... GlobalSchema>
std::any mask_row_idx(
        const attr_type<GlobalSchema...>& attributes,
        const HashableRow<GlobalSchema...>& row,
        std::size_t index) {
    if (attributes[index]) {
        return row.data[index];
    } else {
        return std::any();
    }
}

template<typename... GlobalSchema, std::size_t... Indices>
HashableRow<GlobalSchema...> mask_row_pack(
        const attr_type<GlobalSchema...>& attributes,
        const HashableRow<GlobalSchema...>& row,
        std::index_sequence<Indices...>) {
    std::array<std::any, sizeof...(GlobalSchema)> data = {mask_row_idx<GlobalSchema...>(attributes, row, Indices)...};
    HashableRow<GlobalSchema...> proj_row = HashableRow<GlobalSchema...>(data);
    return proj_row;
}

template<typename... GlobalSchema>
HashableRow<GlobalSchema...> mask_row(
        const attr_type<GlobalSchema...>& attributes,
        const HashableRow<GlobalSchema...>& row) {
    return mask_row_pack(attributes, row, std::make_index_sequence<sizeof...(GlobalSchema)>{});
}

// row merge
template<typename... GlobalSchema>
std::any join_rows_idx(
        const HashableRow<GlobalSchema...>& row_X, 
        const HashableRow<GlobalSchema...>& row_Y, 
        const attr_type<GlobalSchema...>& attributes_X, 
        const attr_type<GlobalSchema...>& attributes_Y,
        std::size_t index) {
    if (attributes_X[index]) {
        return row_X.data[index];
    } else if (attributes_Y[index]) {
        return row_Y.data[index];
    } else {
        return std::any();
    }
}

template<typename... GlobalSchema, std::size_t... Indices>
HashableRow<GlobalSchema...> join_rows_pack(
        const HashableRow<GlobalSchema...>& row_X, 
        const HashableRow<GlobalSchema...>& row_Y, 
        const attr_type<GlobalSchema...>& attributes_X, 
        const attr_type<GlobalSchema...>& attributes_Y,
        std::index_sequence<Indices...>) {
    std::array<std::any, sizeof...(GlobalSchema)> data = 
        {join_rows_idx<GlobalSchema...>(row_X, row_Y, attributes_X, attributes_Y, Indices)...};
    HashableRow<GlobalSchema...> join_row = HashableRow<GlobalSchema...>(data);
    return join_row;
}

template<typename... GlobalSchema>
HashableRow<GlobalSchema...> join_rows(
        const HashableRow<GlobalSchema...>& row_X, 
        const HashableRow<GlobalSchema...>& row_Y, 
        const attr_type<GlobalSchema...>& attributes_X, 
        const attr_type<GlobalSchema...>& attributes_Y) {
    return join_rows_pack(row_X, row_Y, attributes_X, attributes_Y, std::make_index_sequence<sizeof...(GlobalSchema)>{});
}

// print
template<std::size_t index, typename... GlobalSchema>
std::string print_idx(const HashableRow<GlobalSchema...>& row) {
    std::stringstream ss;
    if (row.data[index].has_value()) {
        ss << safe_get<index, GlobalSchema...>(row.data);
    } else {
        ss << "null";
    }
    return ss.str();
}

template<std::size_t... Indices, typename... GlobalSchema>
void print_pack(const HashableRow<GlobalSchema...>& row, std::index_sequence<Indices...>) {
    std::array<std::string, sizeof...(GlobalSchema)> strs = {print_idx<Indices, GlobalSchema...>(row)...};
    for (const std::string& entry : strs) {
        std::cout << entry << " ";
    }
}

template<typename... GlobalSchema>
void print(const HashableRow<GlobalSchema...>& row) {
    print_pack(row, std::make_index_sequence<sizeof...(GlobalSchema)>{});
}
