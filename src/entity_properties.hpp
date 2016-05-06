#pragma once

#include "types.hpp"
#include "hash.hpp"
#include "context.hpp"

namespace boken {

//! common entity properties
enum class entity_property : uint32_t {
    is_player = djb2_hash_32c("is_player")
};

inline constexpr entity_property_id property(entity_property const p) noexcept {
    using type = std::underlying_type_t<entity_property>;
    return entity_property_id {static_cast<type>(p)};
}

string_view name_of(const_context ctx, const_entity_descriptor e) noexcept;

std::string name_of_decorated(const_context ctx, const_entity_descriptor e);

} // namespace boken
