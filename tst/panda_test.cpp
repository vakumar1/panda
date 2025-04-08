#include <gtest/gtest.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "src/panda.h"
#include "src/model/row.h"
#include "tst/test_utils.h"

// Helper function to verify subproblem transformations
template<typename K>
void verify_diffs(
    const std::unordered_map<K, int>& actual_diffs,
    const std::unordered_map<K, int>& expected_diffs) {

    for (const auto& [key, expected_diff] : expected_diffs) {
        EXPECT_GT(actual_diffs.count(key), 0);
        EXPECT_EQ(expected_diff, actual_diffs.at(key));
    }

    for (const auto& [key, actual_diff] : actual_diffs) {
        EXPECT_GT(expected_diffs.count(key), 0);
    }
}

template<typename... GlobalSchema>
void verify_subproblem_transformation(
    const Subproblem<GlobalSchema...>& initial_subproblem,
    const Subproblem<GlobalSchema...>& transformed_subproblem,
    const std::unordered_map<OutputAttributes<GlobalSchema...>, int>& expected_diff_Z,
    const std::unordered_map<Monotonicity<GlobalSchema...>, int>& expected_diff_D,
    const std::unordered_map<Monotonicity<GlobalSchema...>, int>& expected_diff_M,
    const std::unordered_map<Submodularity<GlobalSchema...>, int>& expected_diff_S) {

    std::unordered_map<Monotonicity<GlobalSchema...>, int> conditional_diff_D;
    std::unordered_map<Monotonicity<GlobalSchema...>, int> unconditional_diff_D;
    for (const auto& [mon, diff] : expected_diff_D) {
        if (mon.attrs_X == NULL_ATTR<GlobalSchema...>) {
            unconditional_diff_D[mon] = diff;
        } else {
            conditional_diff_D[mon] = diff;
        }
    }
    
    // set up diffs
    std::unordered_map<OutputAttributes<GlobalSchema...>, int> Z_diffs;
    for (const auto& [output_attrs, count] : initial_subproblem.Z) {
        unsigned transformed_count = transformed_subproblem.Z.count(output_attrs) > 0 ? transformed_subproblem.Z.at(output_attrs) : 0;
        Z_diffs[output_attrs] = transformed_count - count;
    }
    for (const auto& [output_attrs, count] : transformed_subproblem.Z) {
        if (initial_subproblem.Z.count(output_attrs) == 0) {
            Z_diffs[output_attrs] = count;
        }
    }

    std::unordered_map<Monotonicity<GlobalSchema...>, int> D_diffs;
    for (const auto& [monotonicity, count] : initial_subproblem.D) {
        unsigned transformed_count = transformed_subproblem.D.count(monotonicity) > 0 ? transformed_subproblem.D.at(monotonicity) : 0;
        D_diffs[monotonicity] = transformed_count - count;
    }
    for (const auto& [monotonicity, count] : transformed_subproblem.D) {
        if (initial_subproblem.D.count(monotonicity) == 0) {
            D_diffs[monotonicity] = count;
        }
    }

    std::unordered_map<Monotonicity<GlobalSchema...>, int> Tn_tables_diffs;
    for (const auto& [monotonicity, tables] : initial_subproblem.Tn_tables) {
        unsigned transformed_count = transformed_subproblem.Tn_tables.count(monotonicity) > 0 ? transformed_subproblem.Tn_tables.at(monotonicity).size() : 0;
        Tn_tables_diffs[monotonicity] = transformed_count - tables.size();
    }
    for (const auto& [monotonicity, tables] : transformed_subproblem.Tn_tables) {
        if (initial_subproblem.Tn_tables.count(monotonicity) == 0) {
            Tn_tables_diffs[monotonicity] = tables.size();
        }
    }

    std::unordered_map<Monotonicity<GlobalSchema...>, int> Tn_dicts_diffs;
    for (const auto& [monotonicity, dicts] : initial_subproblem.Tn_dicts) {
        unsigned transformed_count = transformed_subproblem.Tn_dicts.count(monotonicity) > 0 ? transformed_subproblem.Tn_dicts.at(monotonicity).size() : 0;
        Tn_dicts_diffs[monotonicity] = transformed_count - dicts.size();
    }
    for (const auto& [monotonicity, dicts] : transformed_subproblem.Tn_dicts) {
        if (initial_subproblem.Tn_dicts.count(monotonicity) == 0) {
            Tn_dicts_diffs[monotonicity] = dicts.size();
        }
    }

    std::unordered_map<Monotonicity<GlobalSchema...>, int> M_diffs;
    for (const auto& [monotonicity, count] : initial_subproblem.M) {
        unsigned transformed_count = transformed_subproblem.M.count(monotonicity) > 0 ? transformed_subproblem.M.at(monotonicity) : 0;
        M_diffs[monotonicity] = transformed_count - count;
    }
    for (const auto& [monotonicity, count] : transformed_subproblem.M) {
        if (initial_subproblem.M.count(monotonicity) == 0) {
            M_diffs[monotonicity] = count;
        }
    }

    std::unordered_map<Submodularity<GlobalSchema...>, int> S_diffs;
    for (const auto& [submodularity, count] : initial_subproblem.S) {
        unsigned transformed_count = transformed_subproblem.S.count(submodularity) > 0 ? transformed_subproblem.S.at(submodularity) : 0;
        S_diffs[submodularity] = transformed_count - count;
    }
    for (const auto& [submodularity, count] : transformed_subproblem.S) {
        if (initial_subproblem.S.count(submodularity) == 0) {
            S_diffs[submodularity] = count;
        }
    }
    verify_diffs(Z_diffs, expected_diff_Z);
    verify_diffs(D_diffs, expected_diff_D);
    verify_diffs(Tn_tables_diffs, unconditional_diff_D);
    verify_diffs(Tn_dicts_diffs, conditional_diff_D);
    verify_diffs(M_diffs, expected_diff_M);
    verify_diffs(S_diffs, expected_diff_S);
}

// Case 1.1: W|0, Y|W in D and N_W * N_Y_W <= B
TEST(GenerateSubproblemSubnodesTest, Case1_1_ConditionMonotonicityWithinBounds) {
    attr_type<int, double, double> W = std::bitset<3>("001");
    attr_type<int, double, double> Y = std::bitset<3>("010");
    Monotonicity<int, double, double> mon_W = {W, NULL_ATTR<int, double, double>};
    Monotonicity<int, double, double> mon_Y_W = {Y, W};

    std::unordered_set<HashableRow<int, double, double>> data_W;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i % 2),
            std::any(),
            std::any()
        };
        HashableRow<int, double, double> row_obj = create_row<int, double, double>(row);
        data_W.insert(row_obj);
    }
    std::unordered_map<HashableRow<int, double, double>, std::unordered_set<HashableRow<int, double, double>>> map_Y_W;
    for (std::size_t i = 0; i < 3; i++) {
        std::array<std::any, 3> key = {
            std::any((int) i),
            std::any(),
            std::any()
        };
        auto key_row = create_row<int, double, double>(key);
        map_Y_W[key_row] = std::unordered_set<HashableRow<int, double, double>>();

        std::array<std::any, 3> value = {
            std::any(),
            std::any((double) 3.0 * i),
            std::any()
        };
        auto value_row = create_row<int, double, double>(value);
        map_Y_W[key_row].insert(value_row);
    }

    Table<int, double, double> table_W{data_W, W};
    BaseDictionary<int, double, double> base_dict_Y_W{map_Y_W, W, Y};
    Dictionary<int, double, double> dict_Y_W = base_dict_Y_W;
    
    std::unordered_map<Monotonicity<int, double, double>, unsigned> D = {
        {mon_W, 1},
        {mon_Y_W, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Table<int, double, double>, Constraint>>> Tn_tables = {
        {mon_W, {{table_W, 2.0}}}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Dictionary<int, double, double>, Constraint>>> Tn_dicts = {
        {mon_Y_W, {{dict_Y_W, 2.0}}}
    };
    Subproblem<int, double, double> initial_subproblem(
        {},
        D,
        Tn_tables,
        Tn_dicts,
        {},
        {},
        5.0
    );
    std::vector<Subproblem<int, double, double>> subnodes = generate_subproblem_subnodes(initial_subproblem);

    ASSERT_EQ(subnodes.size(), 1);
    Monotonicity<int, double, double> mon_YW = {Y ^ W, NULL_ATTR<int, double, double>};
    std::unordered_map<Monotonicity<int, double, double>, int> expected_diff_D = {
        {mon_W, -1},
        {mon_Y_W, -1},
        {mon_YW, 1}
    };
    verify_subproblem_transformation(
        initial_subproblem,
        subnodes[0],
        {},
        expected_diff_D,
        {},
        {}
    );
}

// Case 1.1: W|0, Y|W in D and N_W * N_Y_W > B
// - Reset Lemma Case 0: YW in Z
TEST(GenerateSubproblemSubnodesTest, Case1_2_ConditionMonotonicityExceedsBounds_ResetCase1) {
    // Test Case 1.2 where reset lemma encounters Case 1 (Y|W in D)
    attr_type<int, double, double> W = std::bitset<3>("001");
    attr_type<int, double, double> Y = std::bitset<3>("010");
    Monotonicity<int, double, double> mon_W = {W, NULL_ATTR<int, double, double>};  // W|0
    Monotonicity<int, double, double> mon_Y_W = {Y, W};  // Y|W
    OutputAttributes<int, double, double> attrs_YW = Y ^ W;
    
    std::unordered_set<HashableRow<int, double, double>> data_W;
    for (std::size_t i = 0; i < 4; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i),
            std::any(),
            std::any()
        };
        data_W.insert(create_row<int, double, double>(row));
    }
    std::unordered_map<HashableRow<int, double, double>, std::unordered_set<HashableRow<int, double, double>>> map_Y_W;
    for (const auto& w_row : data_W) {
        map_Y_W[w_row] = std::unordered_set<HashableRow<int, double, double>>();
        for (std::size_t i = 0; i < 3; i++) {
            std::array<std::any, 3> value = {
                std::any(),
                std::any((double) i),
                std::any()
            };
            map_Y_W[w_row].insert(create_row<int, double, double>(value));
        }
    }
    Table<int, double, double> table_W{data_W, W};
    BaseDictionary<int, double, double> base_dict_Y_W{map_Y_W, W, Y};
    Dictionary<int, double, double> dict_Y_W = base_dict_Y_W;

    std::unordered_map<OutputAttributes<int, double, double>, unsigned> Z = {
        {attrs_YW, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, unsigned> D = {
        {mon_W, 1},
        {mon_Y_W, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Table<int, double, double>, Constraint>>> Tn_tables = {
        {mon_W, {{table_W, 4.0}}}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Dictionary<int, double, double>, Constraint>>> Tn_dicts = {
        {mon_Y_W, {{dict_Y_W, 3.0}}}
    };
    
    Subproblem<int, double, double> initial_subproblem(
        Z,
        D,
        Tn_tables,
        Tn_dicts,
        {},
        {},
        10.0
    );
    std::vector<Subproblem<int, double, double>> subnodes = generate_subproblem_subnodes(initial_subproblem);
    
    ASSERT_EQ(subnodes.size(), 1);
    std::unordered_map<OutputAttributes<int, double, double>, int> expected_diff_Z = {
        {attrs_YW, -1}
    };
    std::unordered_map<Monotonicity<int, double, double>, int> expected_diff_D = {
        {mon_W, -1},
        {mon_Y_W, -1}
    };
    
    verify_subproblem_transformation(
        initial_subproblem,
        subnodes[0],
        expected_diff_Z,
        expected_diff_D,
        {},
        {}
    );
}

// Case 1.1: W|0, Y|W in D and N_W * N_Y_W > B
// - Reset Lemma inductive case --> remove YW
//   - Case 2: YW = AB and B|A in M --> remove A
//   - Case 0: A in Z
TEST(GenerateSubproblemSubnodesTest, Case1_2_ConditionMonotonicityExceedsBounds_ResetCase2) {
    attr_type<int, double, double> A = std::bitset<3>("010");
    attr_type<int, double, double> B = std::bitset<3>("101");
    attr_type<int, double, double> W = std::bitset<3>("110");
    attr_type<int, double, double> Y = std::bitset<3>("001");
    Monotonicity<int, double, double> mon_W = {W, NULL_ATTR<int, double, double>};
    Monotonicity<int, double, double> mon_Y_W = {Y, W};
    Monotonicity<int, double, double> mon_B_A = {B, A};
    
    std::unordered_set<HashableRow<int, double, double>> data_W;
    for (std::size_t i = 0; i < 4; i++) {
        std::array<std::any, 3> row = {
            std::any(),
            std::any((double) i),
            std::any((double) 2 * i)
        };
        data_W.insert(create_row<int, double, double>(row));
    }
    std::unordered_map<HashableRow<int, double, double>, std::unordered_set<HashableRow<int, double, double>>> map_Y_W;
    for (const auto& w_row : data_W) {
        map_Y_W[w_row] = std::unordered_set<HashableRow<int, double, double>>();
        for (std::size_t i = 0; i < 3; i++) {
            std::array<std::any, 3> value = {
                std::any((int) i),
                std::any(),
                std::any()
            };
            map_Y_W[w_row].insert(create_row<int, double, double>(value));
        }
    }
    Table<int, double, double> table_W{data_W, W};
    BaseDictionary<int, double, double> base_dict_Y_W{map_Y_W, W, Y};
    Dictionary<int, double, double> dict_Y_W = base_dict_Y_W;
    
    std::unordered_map<OutputAttributes<int, double, double>, unsigned> Z = {
        {A, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, unsigned> D = {
        {mon_W, 1},
        {mon_Y_W, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, unsigned> M = {
        {mon_B_A, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Table<int, double, double>, Constraint>>> Tn_tables = {
        {mon_W, {{table_W, 4.0}}}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Dictionary<int, double, double>, Constraint>>> Tn_dicts = {
        {mon_Y_W, {{dict_Y_W, 3.0}}}
    };
    Subproblem<int, double, double> initial_subproblem(
        Z,
        D,
        Tn_tables,
        Tn_dicts,
        M,
        {},
        10.0
    );
    std::vector<Subproblem<int, double, double>> subnodes = generate_subproblem_subnodes(initial_subproblem);
    
    ASSERT_EQ(subnodes.size(), 1);
    std::unordered_map<OutputAttributes<int, double, double>, int> expected_diff_Z = {
        {A, -1}
    };
    std::unordered_map<Monotonicity<int, double, double>, int> expected_diff_D = {
        {mon_W, -1},
        {mon_Y_W, -1}
    };
    std::unordered_map<Monotonicity<int, double, double>, int> expected_diff_M = {
        {mon_B_A, -1}
    };
    verify_subproblem_transformation(
        initial_subproblem,
        subnodes[0],
        expected_diff_Z,
        expected_diff_D,
        expected_diff_M,
        {}
    );
}

// Case 2: XY|0 in D and Y|X in M
TEST(GenerateSubproblemSubnodesTest, Case2_SplitMonotonicity) {
    attr_type<int, double, double> X = std::bitset<3>("001");
    attr_type<int, double, double> Y = std::bitset<3>("010");
    Monotonicity<int, double, double> mon_XY = {X ^ Y, NULL_ATTR<int, double, double>};
    Monotonicity<int, double, double> mon_Y_X = {Y, X};

    std::unordered_set<HashableRow<int, double, double>> data_XY;
    for (std::size_t i = 0; i < 4; i++) {
        std::array<std::any, 3> row = {
            std::any((int) i),
            std::any((double) i * 2.0),
            std::any()
        };
        data_XY.insert(create_row<int, double, double>(row));
    }
    Table<int, double, double> table_XY{data_XY, X ^ Y};
    
    std::unordered_map<Monotonicity<int, double, double>, unsigned> D = {
        {mon_XY, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, unsigned> M = {
        {mon_Y_X, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Table<int, double, double>, Constraint>>> Tn_tables = {
        {mon_XY, {{table_XY, 4.0}}}
    };
    Subproblem<int, double, double> initial_subproblem(
        {},
        D,
        Tn_tables,
        {},
        M,
        {},
        10.0
    );
    std::vector<Subproblem<int, double, double>> subnodes = generate_subproblem_subnodes(initial_subproblem);
    
    ASSERT_EQ(subnodes.size(), 1);
    Monotonicity<int, double, double> mon_X = {X, NULL_ATTR<int, double, double>};
    std::unordered_map<Monotonicity<int, double, double>, int> expected_diff_D = {
        {mon_XY, -1},
        {mon_X, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, int> expected_diff_M = {
        {mon_Y_X, -1}
    };
    verify_subproblem_transformation(
        initial_subproblem,
        subnodes[0],
        {},
        expected_diff_D,
        expected_diff_M,
        {}
    );
}

// Case 3: XY|0 in D and Y;Z|X in S
TEST(GenerateSubproblemSubnodesTest, Case3_PartitionSubmodularity) {
    attr_type<int, double, double> X = std::bitset<3>("001");
    attr_type<int, double, double> Y = std::bitset<3>("010");
    attr_type<int, double, double> Z = std::bitset<3>("100");
    Monotonicity<int, double, double> mon_XY = {X ^ Y, NULL_ATTR<int, double, double>};
    Submodularity<int, double, double> sub_YZ_X = {Y, Z, X};
    
    std::unordered_set<HashableRow<int, double, double>> data_XY;
    for (std::size_t i = 0; i < 2; i++) {
        for (std::size_t j = 0; j < 3; j++) {
            std::array<std::any, 3> row = {
                std::any((int) i),
                std::any((double) j),
                std::any()
            };
            data_XY.insert(create_row<int, double, double>(row));
        }
    }
    Table<int, double, double> table_XY{data_XY, X ^ Y};
    
    std::unordered_map<Monotonicity<int, double, double>, unsigned> D = {
        {mon_XY, 1}
    };
    std::unordered_map<Submodularity<int, double, double>, unsigned> S = {
        {sub_YZ_X, 1}
    };
    std::unordered_map<Monotonicity<int, double, double>, std::vector<std::pair<Table<int, double, double>, Constraint>>> Tn_tables = {
        {mon_XY, {{table_XY, 4.0}}}
    };
    Subproblem<int, double, double> initial_subproblem(
        {},
        D,
        Tn_tables,
        {},
        {},
        S,
        10.0
    );
    std::vector<Subproblem<int, double, double>> subnodes = generate_subproblem_subnodes(initial_subproblem);
    
    ASSERT_EQ(subnodes.size(), 2);
    
    Monotonicity<int, double, double> mon_X = {X, NULL_ATTR<int, double, double>};
    Monotonicity<int, double, double> mon_Y_XZ = {Y, X ^ Z};
    std::unordered_map<Monotonicity<int, double, double>, int> expected_diff_D = {
        {mon_XY, -1},
        {mon_X, 1},
        {mon_Y_XZ, 1}
    };
    std::unordered_map<Submodularity<int, double, double>, int> expected_diff_S = {
        {sub_YZ_X, -1}
    };
    for (const auto& subnode : subnodes) {
        verify_subproblem_transformation(
            initial_subproblem,
            subnode,
            {},
            expected_diff_D,
            {},
            expected_diff_S
        );
    }
} 
