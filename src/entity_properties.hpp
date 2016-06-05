#pragma once

#include "context_fwd.hpp"

#include <cstdint>


//=====--------------------------------------------------------------------=====
//                            Forward Declarations
//=====--------------------------------------------------------------------=====
namespace boken {

template <typename, typename> class tagged_value;

struct tag_id_property_entity;

using entity_property_id    = tagged_value<uint32_t, tag_id_property_entity>;
using entity_property_value = uint32_t;

}
//=====--------------------------------------------------------------------=====

namespace boken {

//! common entity properties
enum class entity_property : uint32_t;

entity_property_value get_property_value_or(
    const_entity_descriptor ent
  , entity_property_id      property
  , entity_property_value   fallback) noexcept;

//! return whether or not an entity is capable of equipping items
bool can_equip(const_entity_descriptor e) noexcept;

} // namespace boken
