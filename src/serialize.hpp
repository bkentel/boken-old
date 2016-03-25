#pragma once

#include "config.hpp"
#include <functional>

namespace boken { struct item_definition; }

namespace boken {

enum class serialize_data_type : uint32_t {
  null, boolean, i32, u32, i64, u64, float_p, string
};

using on_finish_item_definition = std::function<
    void (item_definition const& definition
    )>;

using on_add_new_item_property = std::function<
    bool (string_view         string
        , uint32_t            hash
        , serialize_data_type type
        , uint32_t            value
    )>;

void load_item_definitions(
    on_finish_item_definition const& on_finish
  , on_add_new_item_property  const& on_property
);

} //namespace boken
