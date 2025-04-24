#include "src/model/table.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

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
    std::vector<std::string> cols;
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return data;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (cols.size() == 0) {
            cols = split(line, ',');
        } else {
            data.push_back(split(line, ','));
        }
    }
    file.close();
    return data;
}

template<typename... GlobalSchema>
Table<GlobalSchema...> parse_table(
    const std::vector<std::vector<std::string>>& raw_rows,
    const std::function<HashableRow<GlobalSchema...>(std::vector<std::string>)> row_parser_fn,
    const attr_type<GlobalSchema...> table_attrs) {
    
    std::unordered_set<HashableRow<GlobalSchema...>> data;
    for (const auto& row : raw_rows) {
        HashableRow<GlobalSchema...> parsed_row = row_parser_fn(row);
        data.insert(parsed_row);
    }
    Table<GlobalSchema...> t{data, table_attrs};
    return t;
}
