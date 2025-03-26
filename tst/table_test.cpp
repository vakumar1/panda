#include <gtest/gtest.h>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <any>

#include "src/model/table.h"

template<typename... GlobalSchema>
HashableRow<GlobalSchema...> create_row(const std::array<std::any, sizeof...(GlobalSchema)>& values) {
    return HashableRow<GlobalSchema...>(values);
}

template<typename... GlobalSchema>
HashableRow<GlobalSchema...> create_projection(
    const std::array<std::any, sizeof...(GlobalSchema)>& values,
    const attr_type<GlobalSchema...>& attrs) {
    
    std::array<std::any, sizeof...(GlobalSchema)> proj_arr;
    for (std::size_t i = 0; i < sizeof...(GlobalSchema); i++) {
        proj_arr[i] = attrs[i] ? values[i] : std::any();
    }
    return create_row<GlobalSchema...>(proj_arr);
}

template<typename... GlobalSchema>
void verify_projection(
    const Table<GlobalSchema...>& projected,
    const Table<GlobalSchema...>& original,
    const attr_type<GlobalSchema...>& proj_attrs) {
    
    EXPECT_EQ(projected.data.size(), original.data.size()) 
        << "Projected table should have same number of rows as original";
    EXPECT_EQ(projected.attributes, proj_attrs) 
        << "Projected table should have correct attributes";
    
    // Verify that each row in the original table has a corresponding projection in the projected table
    for (const auto& original_row : original.data) {
        auto expected_proj = create_projection<GlobalSchema...>(original_row.data, proj_attrs);
        EXPECT_TRUE(projected.data.count(expected_proj) > 0) 
            << "Expected projection not found in projected table";
    }
}

template<typename... GlobalSchema>
void verify_dictionary_construction(
    const Table<GlobalSchema...>& original_table,
    const Dictionary<GlobalSchema...>& dict,
    const attr_type<GlobalSchema...>& attrs_X,
    const attr_type<GlobalSchema...>& attrs_Y) {
    
    EXPECT_EQ(dict.attributes_X, attrs_X);
    EXPECT_EQ(dict.attributes_Y, attrs_Y);

    size_t actual_size = 0;
    for (const auto& [key, value] : dict.construction_map) {
        actual_size += value.size();
    }
    EXPECT_EQ(actual_size, original_table.data.size());
    
    // Verify that for each original row:
    // 1. Its X projection exists as a key in the construction map
    // 2. Its Y projection exists in the corresponding value set
    for (const auto& original_row : original_table.data) {
        auto x_proj = create_projection<GlobalSchema...>(original_row.data, attrs_X);
        auto y_proj = create_projection<GlobalSchema...>(original_row.data, attrs_Y);
        
        EXPECT_TRUE(dict.construction_map.count(x_proj) > 0) 
            << "Failed to find X projection in construction map";
        
        const auto& value_set = dict.construction_map.at(x_proj);
        EXPECT_TRUE(value_set.count(y_proj) > 0) 
            << "Failed to find Y projection in value set";
    }
}

template<typename... GlobalSchema>
void verify_join_result(
    const Table<GlobalSchema...>& joined,
    const Table<GlobalSchema...>& original_table,
    const Dictionary<GlobalSchema...>& dict) {
    
    EXPECT_EQ(joined.attributes, dict.attributes_X ^ dict.attributes_Y) 
        << "Joined table should have joined dict attrs (combining table attributes with dictionary Y attributes)";
    
    // Calculate expected number of rows by summing up Y values for each matching X
    size_t expected_size = 0;
    for (const auto& original_row : original_table.data) {        
        if (dict.construction_map.count(original_row.data) > 0) {
            expected_size += dict.construction_map.at(original_row.data).size();
        }
    }
    EXPECT_EQ(joined.data.size(), expected_size) 
        << "Expected " << expected_size << " rows in joined result";
    
    // For each row in the joined table, verify its components
    for (const auto& joined_row : joined.data) {
        auto x_proj = create_projection<GlobalSchema...>(joined_row.data, dict.attributes_X);
        auto y_proj = create_projection<GlobalSchema...>(joined_row.data, dict.attributes_Y);
        EXPECT_TRUE(original_table.data.count(x_proj) > 0) 
            << "X projection from joined row not found in original table";
        EXPECT_TRUE(dict.construction_map.count(x_proj) > 0) 
            << "X projection not found in dictionary";
        EXPECT_TRUE(dict.construction_map.at(x_proj).count(y_proj) > 0) 
            << "Y projection not found in dictionary value set for X";
    }
}

TEST(TableTest, ProjectFirstTwoColumnsTest) {
    std::unordered_set<HashableRow<int, double, double>> data;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i),
            std::any((double) 2.0 * i),
            std::any((double) 3.0 * i)
        };
        data.insert(create_row<int, double, double>(row));
    }
    
    Table<int, double, double> table{data, std::bitset<3>("111")};
    Table<int, double, double> proj = project(table, std::bitset<3>("011"));
    verify_projection(proj, table, std::bitset<3>("011"));
}

TEST(TableTest, ProjectSingleColumnTest) {
    std::unordered_set<HashableRow<int, double, double>> data;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i),
            std::any((double) 2.0 * i),
            std::any((double) 3.0 * i)
        };
        data.insert(create_row<int, double, double>(row));
    }
    
    Table<int, double, double> table{data, std::bitset<3>("111")};
    Table<int, double, double> proj = project(table, std::bitset<3>("100"));
    verify_projection(proj, table, std::bitset<3>("100"));
}

TEST(TableTest, BasicConstructionTest) {
    std::unordered_set<HashableRow<int, double, double>> data;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i),
            std::any((double) 2.0 * i),
            std::any((double) 3.0 * i)
        };
        data.insert(create_row<int, double, double>(row));
    }
    
    Table<int, double, double> table{data, std::bitset<3>("111")};
    auto attrs_X = std::bitset<3>("011");
    auto attrs_Y = std::bitset<3>("100");
    Dictionary<int, double, double> dict = construction(table, attrs_X, attrs_Y);
    verify_dictionary_construction(table, dict, attrs_X, attrs_Y);
}

TEST(TableTest, OverlappingConstructionTest) {
    std::unordered_set<HashableRow<int, double, double>> data;
    std::array<std::any, 3> row1 = {
        std::any((int) 5),
        std::any((double) 2.5),
        std::any((double) 10.0)
    };
    std::array<std::any, 3> row2 = {
        std::any((int) 5),
        std::any((double) 2.5),
        std::any((double) 20.0)
    };
    data.insert(create_row<int, double, double>(row1));
    data.insert(create_row<int, double, double>(row2));
    
    Table<int, double, double> table{data, std::bitset<3>("111")};
    auto attrs_X = std::bitset<3>("011");
    auto attrs_Y = std::bitset<3>("100");
    Dictionary<int, double, double> dict = construction(table, attrs_X, attrs_Y);
    verify_dictionary_construction(table, dict, attrs_X, attrs_Y);
}

TEST(TableTest, BasicJoinTest) {
    std::unordered_set<HashableRow<int, double, double>> data1;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i),
            std::any((double) 2.0 * i),
            std::any()
        };
        data1.insert(create_row<int, double, double>(row));
    }
    Table<int, double, double> table1{data1, std::bitset<3>("011")};
    
    std::unordered_map<HashableRow<int, double, double>, std::unordered_set<HashableRow<int, double, double>>> map;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> key = {
            std::any((int) i),
            std::any((double) 2.0 * i),
            std::any()
        };
        auto key_row = create_row<int, double, double>(key);
        map[key_row] = std::unordered_set<HashableRow<int, double, double>>();

        std::array<std::any, 3> value = {
            std::any(),
            std::any(),
            std::any((double) 3.0 * i)
        };
        auto value_row = create_row<int, double, double>(value);
        map[key_row].insert(value_row);
    }
    Dictionary<int, double, double> dict{map, std::bitset<3>("011"), std::bitset<3>("100")};
    Table<int, double, double> joined = join(table1, dict);
    verify_join_result(joined, table1, dict);
}

TEST(TableTest, MultipleYJoinTest) {
    std::unordered_set<HashableRow<int, double, double>> data1;
    for (std::size_t i = 0; i < 2; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i),
            std::any((double) 2.0 * i),
            std::any()
        };
        data1.insert(create_row<int, double, double>(row));
    }
    Table<int, double, double> table1{data1, std::bitset<3>("011")};
    
    std::unordered_map<HashableRow<int, double, double>, std::unordered_set<HashableRow<int, double, double>>> map;
    for (std::size_t i = 0; i < 2; i++) {
        std::array<std::any, 3> key = {
            std::any((int) i),
            std::any((double) 2.0 * i),
            std::any()
        };
        auto key_row = create_row<int, double, double>(key);
        map[key_row] = std::unordered_set<HashableRow<int, double, double>>();
        
        for (std::size_t j = 0; j < 3; j++) {
            std::array<std::any, 3> value = {
                std::any(),
                std::any(),
                std::any((double) 10.0 * i + j)
            };
            auto value_row = create_row<int, double, double>(value);
            map[key_row].insert(value_row);
        }
    }
    Dictionary<int, double, double> dict{map, std::bitset<3>("011"), std::bitset<3>("100")};
    Table<int, double, double> joined = join(table1, dict);
    verify_join_result(joined, table1, dict);
}

TEST(TableTest, ExtensionTest) {
    std::unordered_map<HashableRow<int, double, double>, std::unordered_set<HashableRow<int, double, double>>> map;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> key = {
            std::any((int) i),
            std::any(),
            std::any()
        };
        std::array<std::any, 3> value = {
            std::any(),
            std::any((double) 2.0 * i),
            std::any()
        };
        map[create_row<int, double, double>(key)].insert(create_row<int, double, double>(value));
    }
    Dictionary<int, double, double> dict{map, std::bitset<3>("100"), std::bitset<3>("010")};
    ExtendedDictionary<int, double, double> ext = extension(dict, std::bitset<3>("100"));
    EXPECT_EQ(ext.attributes_X, std::bitset<3>("100"));
    EXPECT_EQ(ext.attributes_Y, std::bitset<3>("010"));
    EXPECT_EQ(ext.attributes_Z, std::bitset<3>("100"));
    EXPECT_EQ(ext.construction_map.size(), 3);
}

TEST(TableTest, EmptyTableTest) {
    std::unordered_set<HashableRow<int, double, double>> empty_data;
    Table<int, double, double> empty_table{empty_data, std::bitset<3>("111")};
    Table<int, double, double> empty_proj = project(empty_table, std::bitset<3>("011"));
    EXPECT_EQ(empty_proj.data.size(), 0);
    Dictionary<int, double, double> empty_dict = construction(empty_table, std::bitset<3>("011"), std::bitset<3>("100"));
    EXPECT_EQ(empty_dict.construction_map.size(), 0);
} 
