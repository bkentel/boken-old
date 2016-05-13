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

item_pile const& items(const_item_descriptor i) noexcept;
item_pile&       items(item_descriptor       i) noexcept;

namespace detail {

string_view impl_can_add_item(const_context ctx
    , const_item_descriptor itm, const_item_descriptor dst) noexcept;

string_view impl_can_add_item(const_context ctx
    , const_entity_descriptor subject, point2i32 subject_p
    , const_item_descriptor itm, const_item_descriptor dst) noexcept;

string_view impl_can_remove_item(const_context ctx
    , const_entity_descriptor subject, point2i32 subject_p
    , const_item_descriptor itm, const_item_descriptor src) noexcept;

} // namespace detail

//! @returns whether @p itm can be added to the contents of @p dst.
template <typename UnaryF>
bool can_add_item(
    const_context           const ctx
  , const_item_descriptor   const itm
  , const_item_descriptor   const dst
  , UnaryF                  const on_fail
) noexcept {
    return not_empty_or(on_fail, detail::impl_can_add_item(ctx, itm, dst));
}

//! @returns whether @p itm can be added to the contents of @p dst by the
//!          @p subject located at @p subject_p on the current level.
template <typename UnaryF>
bool can_add_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
  , const_item_descriptor   const itm
  , const_item_descriptor   const dst
  , UnaryF                  const on_fail
) noexcept {
    return not_empty_or(on_fail
      , detail::impl_can_add_item(ctx, subject, subject_p, itm, dst));
}

template <typename UnaryF>
bool can_remove_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
  , const_item_descriptor   const itm
  , const_item_descriptor   const src
  , UnaryF                  const on_fail
) noexcept {
    return not_empty_or(on_fail
      , detail::impl_can_remove_item(ctx, subject, subject_p, itm, src));
}

void merge_into_pile(context ctx, unique_item itm_ptr, item_descriptor itm
    , item_descriptor dst);

//! get the item id to use for the display of item piles.
item_id get_pile_id(game_database const& db) noexcept;

//! get the item id to display an item pile
//! @pre @p pile is not empty
item_id get_pile_id(const_context ctx, item_pile const& pile, item_id pile_id) noexcept;

} //namespace boken
