#pragma once

#include "types.hpp"
#include "hash.hpp"

namespace boken {

//! common entity properties
enum class entity_property : uint32_t {
    is_player = djb2_hash_32c("is_player")
};

inline constexpr entity_property_id property(entity_property const p) noexcept {
    using type = std::underlying_type_t<entity_property>;
    return entity_property_id {static_cast<type>(p)};
}

} // namespace boken
