#include "src/model/table.h"
#include "src/model/panda.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <type_traits>

std::vector<std::string> split(
    const std::string& line,
    const char delimiter) {
    
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

std::vector<std::vector<std::string>> load_csv(const std::string& filename) {
    std::vector<std::vector<std::string>> rows;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::string err_msg = "Could not open file: " + filename;
        throw std::runtime_error(err_msg);
    }
    std::string line;
    while (std::getline(file, line)) {
        rows.push_back(split(line, ','));
    }
    file.close();
    return rows;
}

// entry parsers - extend to support more types
template<typename col_type>
std::any process_entry(std::string entry);

template<>
std::any process_entry<std::string>(std::string entry) {
    return std::any(entry);
}

template<>
std::any process_entry<double>(std::string entry) {
    return std::any(std::stod(entry));
}

template<>
std::any process_entry<int>(std::string entry) {
    return std::any(std::stoi(entry));
}

template<std::size_t index, typename... GlobalSchema>
std::any process_row_idx(
        const std::vector<std::string> row,
        const attr_type<GlobalSchema...> table_attrs) {

    if (table_attrs[index]) {
        return process_entry<elem_type<index, GlobalSchema...>>(row[index]);
    } else {
        return std::any();
    }
}

template<std::size_t... Indices, typename... GlobalSchema>
HashableRow<GlobalSchema...> process_row_pack(
        const std::vector<std::string> row,
        const attr_type<GlobalSchema...> table_attrs,
        const Dummy<GlobalSchema...> dummy,
        std::index_sequence<Indices...>) {
    
    std::vector<std::string> padded_row;
    std::size_t row_idx = 0;
    for (std::size_t i = 0; i < table_attrs.size(); i++) {
        if (table_attrs[i]) {
            padded_row.push_back(row[row_idx]);
            row_idx++;
        } else {
            padded_row.push_back(std::string());
        }
    }
    std::array<std::any, sizeof...(GlobalSchema)> parsed_row = {process_row_idx<Indices, GlobalSchema...>(padded_row, table_attrs)...};
    return HashableRow<GlobalSchema...>(parsed_row);
}

template<typename... GlobalSchema>
HashableRow<GlobalSchema...> process_row(
        const std::vector<std::string> row,
        const attr_type<GlobalSchema...> table_attrs,
        const Dummy<GlobalSchema...> dummy) {
    return process_row_pack(row, table_attrs, dummy, std::index_sequence_for<GlobalSchema...>{});
}

template<typename... GlobalSchema>
Table<GlobalSchema...> parse_table(
        const std::vector<std::vector<std::string>>& raw_rows,
        const attr_type<GlobalSchema...> table_attrs,
        const Dummy<GlobalSchema...> dummy) {
    std::unordered_set<HashableRow<GlobalSchema...>> data;
    for (std::size_t i = 1; i < raw_rows.size(); i++) {
        HashableRow<GlobalSchema...> parsed_row = process_row(raw_rows[i], table_attrs, dummy);
        data.insert(parsed_row);
    }
    Table<GlobalSchema...> t{data, table_attrs};
    return t;
}


template<typename... GlobalSchema>
Subproblem<GlobalSchema...> parse_spec(
    const std::string& spec_dir,
    const std::string& spec_file_path,
    const std::string& tables_dir) {

    // Load YAML spec file
    YAML::Node spec;
    try {
        spec = YAML::LoadFile(spec_dir + "/" + spec_file_path);
    } catch (const YAML::ParserException& e) {
        std::cerr << "Error parsing YAML file: " << e.what() << std::endl;
        throw e;
    }

    // Extract global schema
    std::vector<std::string> global_schema_names = spec["GlobalSchema"].as<std::vector<std::string>>();
    std::unordered_map<std::string, attr_type<GlobalSchema...>> column_to_attr;
    std::size_t num_columns = global_schema_names.size();
    for (std::size_t i = 0; i < num_columns; ++i) {
        std::string col_name = global_schema_names[i];
        attr_type<GlobalSchema...> attr;
        attr.set(i);
        column_to_attr[col_name] = attr;
    }

    // Load tables + constraints and calculate global bound
    YAML::Node tables = spec["Tables"];
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Table<GlobalSchema...>, Constraint>>> Tn_tables;
    long double global_bound = 1;
    for (std::size_t i = 0; i < tables.size(); i++) {
        std::vector<std::string> table_entry = tables[i].as<std::vector<std::string>>();
        std::string table_file_name = table_entry[0];
        Constraint constraint = std::stod(table_entry[1]);
        long double w = std::stod(table_entry[2]);

        // read table file into raw rows
        std::vector<std::vector<std::string>> raw_rows = load_csv(tables_dir + "/" + table_file_name);
        if (raw_rows.empty()) {
            std::string err_msg = "Empty table file " + table_file_name;
            throw std::runtime_error(err_msg);
        }
        std::vector<std::string> csv_columns = raw_rows[0];
        attr_type<GlobalSchema...> table_attrs;
        for (const auto& col_name : csv_columns) {
            if (column_to_attr.find(col_name) != column_to_attr.end()) {
                table_attrs ^= column_to_attr.at(col_name);
            } else {
                std::string err_msg = "Column not found in global schema: " + col_name;
                throw std::runtime_error(err_msg);
            }
        }

        // Parse the table and store with constraint
        Table<GlobalSchema...> table = parse_table(raw_rows, table_attrs, Dummy<GlobalSchema...>());
        Monotonicity<GlobalSchema...> mon = {
            table_attrs,
            NULL_ATTR<GlobalSchema...>
        };
        if (Tn_tables.count(mon) == 0) {
            Tn_tables[mon] = {};
        }
        Tn_tables[mon].push_back(std::make_pair(table, constraint));

        // update the global bound
        global_bound *= (pow(constraint, w));
    }

    // Construct Z
    std::unordered_map<OutputAttributes<GlobalSchema...>, unsigned> Z;
    YAML::Node output_queries = spec["OutputAttributes"];
    for (std::size_t i = 0; i < output_queries.size(); ++i) {
        std::vector<std::string> query_columns = output_queries[i].as<std::vector<std::string>>();
        OutputAttributes<GlobalSchema...> output_attrs;
        for (const auto& col_name : query_columns) {
            if (column_to_attr.count(col_name) > 0) {
                output_attrs = output_attrs | column_to_attr.at(col_name);
            } else {
                std::string err_msg = "Column not found in global schema: " + col_name;
                throw std::runtime_error(err_msg);
            }
        }
        Z[output_attrs] = 1;
    }

    // Construct D
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> D;
    for (const auto& [mon, ts] : Tn_tables) {
        D[mon] = ts.size();
    }

    // Construct Tn_dicts (empty)
    std::unordered_map<Monotonicity<GlobalSchema...>, std::vector<std::pair<Dictionary<GlobalSchema...>, Constraint>>> Tn_dicts;

    // Construct M
    std::unordered_map<Monotonicity<GlobalSchema...>, unsigned> M;
    YAML::Node monotonicities = spec["M"];
    for (std::size_t i = 0; i < monotonicities.size(); ++i) {
        YAML::Node mono = monotonicities[i];
        std::vector<std::string> Y_cols = mono[0].as<std::vector<std::string>>();
        std::vector<std::string> X_cols = mono[1].as<std::vector<std::string>>();
        unsigned value = mono[2].as<unsigned>();

        Monotonicity<GlobalSchema...> monotonicity;
        for (const auto& col_name : Y_cols) {
            if (column_to_attr.count(col_name) > 0) {
                monotonicity.attrs_Y = monotonicity.attrs_Y | column_to_attr.at(col_name);
            } else {
                std::string err_msg = "Column not found in global schema: " + col_name;
                throw std::runtime_error(err_msg);
            }
        }
        for (const auto& col_name : X_cols) {
            if (column_to_attr.count(col_name) > 0) {
                monotonicity.attrs_X = monotonicity.attrs_X | column_to_attr.at(col_name);
            } else {
                std::string err_msg = "Column not found in global schema: " + col_name;
                throw std::runtime_error(err_msg);
            }
        }
        M[monotonicity] = value;
    }

    // Construct S
    std::unordered_map<Submodularity<GlobalSchema...>, unsigned> S;
    YAML::Node submodularities = spec["S"];
    for (size_t i = 0; i < submodularities.size(); ++i) {
        YAML::Node sub = submodularities[i];
        std::vector<std::string> Y_cols = sub[0].as<std::vector<std::string>>();
        std::vector<std::string> Z_cols = sub[1].as<std::vector<std::string>>();
        std::vector<std::string> X_cols = sub[2].as<std::vector<std::string>>();
        unsigned value = sub[3].as<unsigned>();

        Submodularity<GlobalSchema...> submodularity;
        for (const auto& col_name : Y_cols) {
            if (column_to_attr.count(col_name) > 0) {
                submodularity.attrs_Y = submodularity.attrs_Y | column_to_attr.at(col_name);
            } else {
                std::string err_msg = "Column not found in global schema: " + col_name;
                throw std::runtime_error(err_msg);
            }
        }
        for (const auto& col_name : Z_cols) {
            if (column_to_attr.count(col_name) > 0) {
                submodularity.attrs_Z = submodularity.attrs_Z | column_to_attr.at(col_name);
            } else {
                std::string err_msg = "Column not found in global schema: " + col_name;
                throw std::runtime_error(err_msg);
            }
        }
        for (const auto& col_name : X_cols) {
            if (column_to_attr.count(col_name) > 0) {
                submodularity.attrs_X = submodularity.attrs_X | column_to_attr.at(col_name);
            } else {
                std::string err_msg = "Column not found in global schema: " + col_name;
                throw std::runtime_error(err_msg);
            }
        }
        S[submodularity] = value;
    }

    return Subproblem<GlobalSchema...>(
        Z,
        D,
        Tn_tables,
        Tn_dicts,
        M,
        S,
        global_bound
    );
}
