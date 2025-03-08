#include "src/model/table.h"

template<typename... XY, size_t X_len>
auto project(Table<std::tuple<XY...>>& table, std::array<size_t, X_len>& global_indices_X) {
    std::array<size_t, X_len> local_indices_X = {};
    size_t curr_X_index = 0;
    for (size_t i = 0; i < table.global_indices.length(); i++) {
        bool is_X = false;
        for (size_t j = 0; j < global_indices_X.length(); j++) {
            if (table.global_indices[i] == global_indices_X[j]) {
                is_X = true;
                break;
            }
        }
        if (is_X) {
            local_indices_X[curr_X_index] = i;
            curr_X_index++;
        }
    }
    for (size_t i = 0; i < X_len; i++) {
        local_indices_X[i] = table.global_to_local_index[global_indices_X[i]];
    }
    auto proj_data_set = {};
    for (const auto& row : table) {
        proj_data_set.insert(std::make_tuple(std::get<local_indices_X>(row)));
    }
    auto proj_data = std::vector<decltype(*proj_data_set.begin())>(proj_data_set.begin(), proj_data_set.end());
    auto proj_table = Table{proj_data, global_indices_X};
    return proj_data;
}

template<typename... X, typename... Y>
auto join(Table<std::tuple<X...>>& table, Dictionary<std::tuple<X...>, std::tuple<Y...>, void>& dictionary) {
    auto X_len = dictionary.global_indices_X.length();
    auto Y_len = dictionary.global_indices_Y.length();
    auto join_data = {};
    for (const auto& x_row : table.data) {
        if (dictionary.construction_map.contains(x_row)) {
            for (const auto& y_row : dictionary.construction_map[x_row]) {
                join_data.push_back(std::tuple_cat(x_row, y_row));
            }
        }
    }
    std::array<size_t, X_len + Y_len> global_indices_XY = {};
    std::copy(dictionary.global_indices_X.begin(), dictionary.global_indices_X.end(),
        global_indices_XY.begin());
    std::copy(dictionary.global_indices_Y.begin(), dictionary.global_indices_Y.end(),
        global_indices_XY.begin() + X_len);
    auto join_table = Table{join_data, global_indices_XY};
    return join_table;
}

template<typename... X, typename... Y, typename... Z>
auto extension(Dictionary<std::tuple<X...>, std::tuple<Y...>, void>& dictionary, std::array<size_t, sizeof...(Z)> global_indices_Z) {
    auto extension_dict = Dictionary<std::tuple<X...>, std::tuple<Y...>, std::tuple<Z...>>{
        dictionary.construction_map,
        dictionary.global_indices_X,
        dictionary.global_indices_Y,
        global_indices_Z
    };
    return extension_dict;
}

template<typename... XY, size_t X_len, size_t Y_len>
auto construction(Table<std::tuple<XY...>> table,
        std::array<size_t, X_len>& global_indices_X,
        std::array<size_t, Y_len>& global_indices_Y) {
    auto construction_map = {};
    std::array<size_t, X_len> local_indices_X = {};
    std::array<size_t, Y_len> local_indices_Y = {};
    size_t curr_X_index = 0;
    size_t curr_Y_index = 0;
    for (size_t i = 0; i < table.global_indices.length(); i++) {
        bool is_X = false;
        for (size_t j = 0; j < global_indices_X.length(); j++) {
            if (table.global_indices[i] == global_indices_X[j]) {
                is_X = true;
                break;
            }
        }
        if (is_X) {
            local_indices_X[curr_X_index] = i;
            curr_X_index++;
        } else {
            local_indices_Y[curr_Y_index] = i;
            curr_Y_index++;
        }
    }
    for (const auto& row : table.data) {
        auto x_row = std::make_tuple(std::get<local_indices_X>(row));
        auto y_row = std::make_tuple(std::get<local_indices_Y>(row));
        if (!construction_map.contains(x_row)) {
            construction_map[x_row] = {};
        }
        construction_map[x_row].insert(y_row);
    }
    auto construction_dict = Dictionary<
        typename decltype(construction_map)::key_type, 
        typename decltype(construction_map)::mapped_type,
        void
    >{
        construction_map,
        global_indices_X,
        global_indices_Y
    };
    return construction_dict;
}
