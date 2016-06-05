#pragma once

#include "context_fwd.hpp"

#include <string>
#include <cstdint>

//=====--------------------------------------------------------------------=====
//                            Forward Declarations
//=====--------------------------------------------------------------------=====
namespace boken {

template <typename, typename> class tagged_value;

struct tag_id_property_item;

using item_property_id    = tagged_value<uint32_t, tag_id_property_item>;
using item_property_value = uint32_t;

}
//=====--------------------------------------------------------------------=====

namespace boken {

//! common item properties
enum class item_property : uint32_t;

item_property_value get_property_value_or(
    const_item_descriptor itm
  , item_property_id      property
  , item_property_value   fallback) noexcept;

//! Get the weight of an item exclusive of any other items that might be
//! contained within it.
int32_t weight_of_exclusive(const_item_descriptor i) noexcept;

//! Get the weight of an item inclusive of the weight of any other items that
//! might be contained within it.
int32_t weight_of_inclusive(const_context ctx, const_item_descriptor i) noexcept;

//! return a positive integer which is the capacity of the item, or 0 otherwise.
uint32_t is_container(const_item_descriptor i) noexcept;

//! return a positive integer indicating the identification state of the item
uint32_t is_identified(const_item_descriptor i) noexcept;

void set_identified(item_descriptor i, uint32_t level);

//! return a positive integer for the current stack size; return 1 for items
//! which cannot stack.
uint32_t current_stack_size(const_item_descriptor i) noexcept;

//! return a detailed description of the item
//! @note This builds a new description every time it is called; it is meant for
//!       generating descriptions to display to the player only.
std::string item_description(const_context ctx, const_item_descriptor i);

//! return whether or not an item is something that can be equipped by an
//! an entity.
bool can_equip(const_item_descriptor i) noexcept;

} // namespace boken
