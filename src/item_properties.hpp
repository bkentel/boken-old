#pragma once

#include "types.hpp"
#include "hash.hpp"

namespace boken {

//! common item properties
enum class item_property : uint32_t {
    weight             = djb2_hash_32c("weight")
  , capacity           = djb2_hash_32c("capacity")
  , stack_size         = djb2_hash_32c("stack_size")
  , current_stack_size = djb2_hash_32c("current_stack_size")
  , identified         = djb2_hash_32c("identified")
};

//! item_property:identified
//! 0 -- unknown
//! 1 -- seen

inline constexpr item_property_id property(item_property const p) noexcept {
    using type = std::underlying_type_t<item_property>;
    return item_property_id {static_cast<type>(p)};
}

} // namespace boken
