#pragma once

#include "types.hpp"
#include "context.hpp"
#include "config.hpp"
#include "item_def.hpp"
#include "object.hpp"

#include <string>

namespace boken { class game_database; }

namespace boken {

class item : public object<item, item_definition, item_instance_id> {
public:
    using object::object;
};

void merge_into_pile(
    context         ctx
  , unique_item     itm_ptr
  , item_descriptor itm
  , item_descriptor pile);

namespace detail {

string_view impl_can_add_item(context ctx, const_item_descriptor itm
                            , const_item_descriptor dest) noexcept;

string_view impl_can_remove_item(context ctx, const_item_descriptor itm
                               , const_item_descriptor dest) noexcept;

} // namespace detail

template <typename UnaryF>
bool can_add_item(
    context               ctx
  , const_item_descriptor itm
  , const_item_descriptor dest
  , UnaryF                on_fail
) noexcept {
    auto const result = detail::impl_can_add_item(ctx, itm, dest);
    return result.empty()
      ? true
      : (on_fail(result), false);
}

template <typename UnaryF>
bool can_remove_item(
    context               ctx
  , const_item_descriptor itm
  , const_item_descriptor dest
  , UnaryF                on_fail
) noexcept {
    auto const result = detail::impl_can_remove_item(ctx, itm, dest);
    return result.empty()
      ? true
      : (on_fail(result), false);
}

string_view name_of(const_context ctx, const_item_descriptor i) noexcept;

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
//! @note This build a new description every time it is called; it is meant for
//!       generating descriptions to display to the player only.
std::string item_description(const_context ctx, const_item_descriptor i);

//! get the item id to use for the display of item piles.
item_id get_pile_id(game_database const& db) noexcept;

//! get the item id to display an item pile
//! @pre @p pile is not empty
item_id get_pile_id(const_context ctx, item_pile const& pile, item_id pile_id) noexcept;

} //namespace boken
