#pragma once

#include "types.hpp"
#include "hash.hpp"

namespace boken {

//! common item properties
enum class item_property : item_property_id::type {
    weight             = djb2_hash_32c("weight")
  , capacity           = djb2_hash_32c("capacity")
  , stack_size         = djb2_hash_32c("stack_size")
  , current_stack_size = djb2_hash_32c("current_stack_size")
};

inline constexpr item_property_id property(item_property const p) noexcept {
    return item_property_id {static_cast<item_property_id::type>(p)};
}

} // namespace boken
