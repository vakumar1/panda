
#include <variant>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <array>
#include <unordered_set>
#include <any>
#include <utility>
#include <bitset>

#include "src/model/row.h"

// model
template<typename... GlobalSchema>
struct Table {
    std::unordered_set<HashableRow<GlobalSchema...>>& data;
    attr_type<GlobalSchema...> attributes;
};

template<typename... GlobalSchema>
struct Dictionary {
    std::unordered_map<HashableRow<GlobalSchema...>, std::unordered_set<HashableRow<GlobalSchema...>>>& construction_map;
    attr_type<GlobalSchema...> attributes_X;
    attr_type<GlobalSchema...> attributes_Y;
};

template<typename... GlobalSchema>
struct ExtendedDictionary {
    std::unordered_map<HashableRow<GlobalSchema...>, std::unordered_set<HashableRow<GlobalSchema...>>>& construction_map;
    attr_type<GlobalSchema...> attributes_X;
    attr_type<GlobalSchema...> attributes_Y;
    attr_type<GlobalSchema...> attributes_Z;
};

// projection
template<typename... GlobalSchema>
Table<GlobalSchema...> project(
        Table<GlobalSchema...>& table,
        attr_type<GlobalSchema...> proj_attrs) {
    std::unordered_set<HashableRow<GlobalSchema...>> proj_data = {};
    for (const HashableRow<GlobalSchema...>& row : table.data) {
        HashableRow<GlobalSchema...> proj_row = mask_row<GlobalSchema...>(proj_attrs, row);
        proj_data.insert(proj_row);
    }
    return Table<GlobalSchema...>{proj_data, proj_attrs};
}

// join
template<typename... GlobalSchema>
Table<GlobalSchema...> join(
        Table<GlobalSchema...>& table,
        Dictionary<GlobalSchema...>& dictionary) {

    // TODO: add runtime precondition that overlap_attrs == 0
    attr_type<GlobalSchema...> join_attrs = dictionary.attributes_X ^ dictionary.attributes_Y;
    attr_type<GlobalSchema...> overlap_attrs = dictionary.attributes_X | dictionary.attributes_Y;

    std::unordered_set<HashableRow<GlobalSchema...>> join_data = {};
    for (const HashableRow<GlobalSchema...>& row_X : table.data) {
        if (dictionary.construction_map.count(row_X) > 0) {
            for (const HashableRow<GlobalSchema...>& row_Y : dictionary.construction_map[row_X]) {
                HashableRow<GlobalSchema...> join_row = 
                    join_rows<GlobalSchema...>(row_X, row_Y, dictionary.attributes_X, dictionary.attributes_Y);
                join_data.insert(join_row);
            }
        }
    }
    return Table<GlobalSchema...>{join_data, join_attrs};
}

// extension
template<typename... GlobalSchema>
ExtendedDictionary<GlobalSchema...> extension(
        Dictionary<GlobalSchema...>& dictionary,
        attr_type<GlobalSchema...> ext_attrs) {

    // TODO: add runtime precondition that overlap_attrs == 0
    attr_type<GlobalSchema...> overlap_attrs = ext_attrs | (dictionary.attributes_X | dictionary.attributes_Y);

    return ExtendedDictionary<GlobalSchema...>{
        dictionary.construction_map,
        dictionary.attributes_X,
        dictionary.attributes_Y,
        ext_attrs
    };
}

// construction
template<typename... GlobalSchema>
Dictionary<GlobalSchema...> construction(
        Table<GlobalSchema...>& table,
        attr_type<GlobalSchema...> attrs_X,
        attr_type<GlobalSchema...> attrs_Y) {

    // TODO: add runtime precondition that overlap_attrs == 0
    attr_type<GlobalSchema...> overlap_attrs = attrs_X | attrs_Y;

    // TODO: add runtime precondition that join_attrs = table attrs
    attr_type<GlobalSchema...> join_attrs = attrs_X ^ attrs_Y;

    std::unordered_map<HashableRow<GlobalSchema...>, std::unordered_set<HashableRow<GlobalSchema...>>> construction_map = {};
    for (const HashableRow<GlobalSchema...>& row : table.data) {
        HashableRow<GlobalSchema...> row_X = mask_row<GlobalSchema...>(attrs_X, row);
        HashableRow<GlobalSchema...> row_Y = mask_row<GlobalSchema...>(attrs_Y, row);
        if (construction_map.count(row_X) == 0) {
            construction_map[row_X] = {};
        }
        construction_map[row_X].insert(row_Y);
    }
    return Dictionary<GlobalSchema...>{
        construction_map,
        attrs_X,
        attrs_Y
    };
}

// print
template<typename... GlobalSchema>
void print(Table<GlobalSchema...> table) {
    for (const HashableRow<GlobalSchema...>& row : table.data) {
        print(row);
        std::cout << std::endl;
    }
}
