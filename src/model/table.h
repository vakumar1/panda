#include <variant>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <array>
#include <unordered_set>
#include <any>
#include <utility>
#include <bitset>
#include <cmath>

#include "src/model/row.h"

// model
template<typename... GlobalSchema>
struct Table {
    std::unordered_set<HashableRow<GlobalSchema...>> data;
    attr_type<GlobalSchema...> attributes;
};

template<typename... GlobalSchema>
struct Dictionary {
    std::unordered_map<HashableRow<GlobalSchema...>, std::unordered_set<HashableRow<GlobalSchema...>>> construction_map;
    attr_type<GlobalSchema...> attributes_X;
    attr_type<GlobalSchema...> attributes_Y;
};

template<typename... GlobalSchema>
struct ExtendedDictionary {
    std::unordered_map<HashableRow<GlobalSchema...>, std::unordered_set<HashableRow<GlobalSchema...>>> construction_map;
    attr_type<GlobalSchema...> attributes_X;
    attr_type<GlobalSchema...> attributes_Y;
    attr_type<GlobalSchema...> attributes_Z;
};

// projection
template<typename... GlobalSchema>
Table<GlobalSchema...> project(
        const Table<GlobalSchema...>& table,
        const attr_type<GlobalSchema...>& proj_attrs) {
    Table<GlobalSchema...> proj_table;
    proj_table.attributes = proj_attrs;
    proj_table.data = {};
    for (const HashableRow<GlobalSchema...>& row : table.data) {
        HashableRow<GlobalSchema...> proj_row = mask_row<GlobalSchema...>(proj_attrs, row);
        proj_table.data.insert(proj_row);
    }
    return proj_table;
}

// join
template<typename... GlobalSchema>
Table<GlobalSchema...> join(
        const Table<GlobalSchema...>& table,
        const Dictionary<GlobalSchema...>& dictionary) {

    // TODO: add runtime precondition that overlap_attrs == 0
    const attr_type<GlobalSchema...> join_attrs = dictionary.attributes_X ^ dictionary.attributes_Y;
    const attr_type<GlobalSchema...> overlap_attrs = dictionary.attributes_X | dictionary.attributes_Y;

    Table<GlobalSchema...> join_table;
    join_table.attributes = join_attrs;
    join_table.data = {};
    for (const HashableRow<GlobalSchema...>& row_X : table.data) {
        if (dictionary.construction_map.count(row_X) > 0) {
            for (const HashableRow<GlobalSchema...>& row_Y : dictionary.construction_map.at(row_X)) {
                HashableRow<GlobalSchema...> join_row = 
                    join_rows<GlobalSchema...>(row_X, row_Y, dictionary.attributes_X, dictionary.attributes_Y);
                join_table.data.insert(join_row);
            }
        }
    }
    return join_table;
}

// extension
template<typename... GlobalSchema>
ExtendedDictionary<GlobalSchema...> extension(
        const Dictionary<GlobalSchema...>& dictionary,
        const attr_type<GlobalSchema...>& ext_attrs) {

    // TODO: add runtime precondition that overlap_attrs == 0
    const attr_type<GlobalSchema...> overlap_attrs = ext_attrs | (dictionary.attributes_X | dictionary.attributes_Y);

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
        const Table<GlobalSchema...>& table,
        const attr_type<GlobalSchema...>& attrs_X,
        const attr_type<GlobalSchema...>& attrs_Y) {

    // TODO: add runtime precondition that overlap_attrs == 0
    const attr_type<GlobalSchema...> overlap_attrs = attrs_X | attrs_Y;

    // TODO: add runtime precondition that join_attrs = table attrs
    const attr_type<GlobalSchema...> join_attrs = attrs_X ^ attrs_Y;

    Dictionary<GlobalSchema...> dict;
    dict.attributes_X = attrs_X;
    dict.attributes_Y = attrs_Y;
    dict.construction_map = {};
    for (const HashableRow<GlobalSchema...>& row : table.data) {
        HashableRow<GlobalSchema...> row_X = mask_row<GlobalSchema...>(attrs_X, row);
        HashableRow<GlobalSchema...> row_Y = mask_row<GlobalSchema...>(attrs_Y, row);
        if (dict.construction_map.count(row_X) == 0) {
            dict.construction_map[row_X] = {};
        }
        dict.construction_map[row_X].insert(row_Y);
    }
    return dict;
}

// partition
template<typename... GlobalSchema>
std::vector<Table<GlobalSchema...>> partition(
        const Table<GlobalSchema...>& table,
        const attr_type<GlobalSchema...>& partition_attrs) {
    std::unordered_map<HashableRow<GlobalSchema...>, std::unordered_set<HashableRow<GlobalSchema...>>> row_X_to_rows;
    for (const HashableRow<GlobalSchema...>& row : table.data) {
        HashableRow<GlobalSchema...> row_X = mask_row<GlobalSchema...>(partition_attrs, row);
        if (row_X_to_rows.count(row_X) == 0) {
            row_X_to_rows[row_X] = {};
        }
        row_X_to_rows[row_X].insert(row);
    }
    unsigned bucket_count = 2 * std::ceil(log2(table.data.size())) + 1;
    std::vector<std::vector<HashableRow<GlobalSchema...>>> partitioned_rows(bucket_count);
    for (const auto& [row_X, rows] : row_X_to_rows) {
        unsigned log_degree = std::ceil(log2(rows.size()));
        for (const HashableRow<GlobalSchema...>& row : rows) {
            partitioned_rows[log_degree].push_back(row);
        }
    }
    std::vector<Table<GlobalSchema...>> partitioned_tables;
    for (std::size_t i = 0; i < bucket_count; i++) {
        Table<GlobalSchema...> partitioned_table1;
        Table<GlobalSchema...> partitioned_table2;
        partitioned_table1.attributes = table.attributes;
        partitioned_table2.attributes = table.attributes;
        partitioned_table1.data = {};
        partitioned_table2.data = {};
        for (std::size_t j = 0; j < partitioned_rows[i].size(); j++) {
            if (j % 2 == 0) {
                partitioned_table1.data.insert(partitioned_rows[i][j]);
            } else {
                partitioned_table2.data.insert(partitioned_rows[i][j]);
            }
        }
        if (partitioned_table1.data.size() > 0) {
            partitioned_tables.push_back(partitioned_table1);
        }
        if (partitioned_table2.data.size() > 0) {
            partitioned_tables.push_back(partitioned_table2);
        }
    }
    return partitioned_tables;
}

// print
template<typename... GlobalSchema>
void print(const Table<GlobalSchema...>& table) {
    std::cout << table.attributes << std::endl;
    for (const HashableRow<GlobalSchema...>& row : table.data) {
        print(row);
        std::cout << std::endl;
    }
}

template<typename... GlobalSchema>
void print(const Dictionary<GlobalSchema...>& dict) {
    std::cout << dict.attributes_X << std::endl;
    std::cout << dict.attributes_Y << std::endl;
    for (const auto& [key, value] : dict.construction_map) {
        print(key);
        std::cout << " -> " << std::endl;
        for (const HashableRow<GlobalSchema...>& row : value) {
            std::cout << "    ";
            print(row);
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}
