#pragma once

#include "context_fwd.hpp"

#include <cstdint>

namespace boken {

//! common entity properties
enum class entity_property : uint32_t;

//! return whether or not an entity is capable of equipping items
bool can_equip(const_entity_descriptor e) noexcept;

} // namespace boken
