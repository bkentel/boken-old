#pragma once

#include "config.hpp"
#include <functional>
#include <cstdint>
#include <cstddef>

namespace boken { struct item_definition; }
namespace boken { struct entity_definition; }

namespace boken {

enum class serialize_data_type : uint32_t {
  null, boolean, i32, u32, i64, u64, float_p, string
};

using on_finish_item_definition = std::function<
    void (item_definition const& definition)>;

using on_add_new_item_property = std::function<
    bool (string_view         string
        , uint32_t            hash
        , serialize_data_type type
        , uint32_t            value
    )>;

using on_finish_entity_definition = std::function<
    void (entity_definition const& definition)>;

using on_add_new_entity_property = on_add_new_item_property;

void load_item_definitions(
    on_finish_item_definition const& on_finish
  , on_add_new_item_property  const& on_property
);

void load_entity_definitions(
    on_finish_entity_definition const& on_finish
  , on_add_new_entity_property  const& on_property
);

uint32_t to_property(nullptr_t n) noexcept;
uint32_t to_property(bool n) noexcept;
uint32_t to_property(int32_t n) noexcept;
uint32_t to_property(uint32_t n) noexcept;
uint32_t to_property(double n) noexcept;
uint32_t to_property(string_view n) noexcept;

} //namespace boken
