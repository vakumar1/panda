
#include <variant>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <array>
#include <set>

#include "src/model/attrs.h"

template<typename X>
struct Table;

template <typename... X>
struct Table<std::tuple<X...>> {
    std::vector<std::tuple<X...>>& data;
    std::array<std::size_t, sizeof...(X)> global_indices;
};

template<typename X, typename Y, typename Z>
struct Dictionary;

template<typename... X, typename...Y>
struct Dictionary<std::tuple<X...>, std::tuple<Y...>, void> {
    std::unordered_map<std::tuple<X...>, std::set<std::tuple<Y...>>>& construction_map;
    std::array<std::size_t, sizeof...(X)> global_indices_X;
    std::array<std::size_t, sizeof...(Y)> global_indices_Y;
};

template<typename... X, typename...Y, typename... Z>
struct Dictionary<std::tuple<X...>, std::tuple<Y...>, std::tuple<Z...>> {
    std::unordered_map<std::tuple<X...>, std::set<std::tuple<Y...>>>& construction_map;
    std::array<std::size_t, sizeof...(X)> global_indices_X;
    std::array<std::size_t, sizeof...(Y)> global_indices_Y;
    std::array<std::size_t, sizeof...(Z)> global_indices_Z;
};

template<typename... XY, size_t X_len>
auto project(Table<std::tuple<XY...>>& table, std::array<size_t, X_len>& global_indices);

template<typename... X, typename... Y>
auto join(Table<std::tuple<X...>>& table, Dictionary<std::tuple<X...>, std::tuple<Y...>, void>& dictionary);

template<typename... X, typename... Y, typename... Z>
auto extension(Dictionary<std::tuple<X...>, std::tuple<Y...>, void>& dictionary, std::array<size_t, sizeof...(Z)> global_indices_Z);

template<typename... XY, size_t X_len, size_t Y_len>
auto construction(Table<std::tuple<XY...>> table,
        std::array<size_t, X_len>& global_indices_X,
        std::array<size_t, Y_len>& global_indices_Y);
