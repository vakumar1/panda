#pragma once

#include "src/model/table.h"
#include "src/model/row.h"

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
