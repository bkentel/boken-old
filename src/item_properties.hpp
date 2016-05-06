#pragma once

#include "types.hpp"
#include "hash.hpp"
#include "context.hpp"

namespace boken {

//! common item properties
enum class item_property : uint32_t {
    weight             = djb2_hash_32c("weight")
  , capacity           = djb2_hash_32c("capacity")
  , stack_size         = djb2_hash_32c("stack_size")
  , current_stack_size = djb2_hash_32c("current_stack_size")
  , identified         = djb2_hash_32c("identified")
};

inline constexpr item_property_id property(item_property const p) noexcept {
    using type = std::underlying_type_t<item_property>;
    return item_property_id {static_cast<type>(p)};
}

//! Get the simple "undecorated" name of an item.
//! For example, "chest" and not "chest [10]"
string_view name_of(const_context ctx, const_item_descriptor i) noexcept;

//! Get the "decorated" name of an item.
//! For example, "chest [10]" and not "chest".
std::string name_of_decorated(const_context ctx, const_item_descriptor itm);

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

//! return a detailed description of the item
//! @note This builds a new description every time it is called; it is meant for
//!       generating descriptions to display to the player only.
std::string item_description(const_context ctx, const_item_descriptor i);

} // namespace boken
