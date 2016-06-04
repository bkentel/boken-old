#pragma once

#include "types.hpp"
#include "context.hpp"
#include "config.hpp"
#include "item_def.hpp"
#include "object.hpp"

#include <string>

namespace boken { class game_database; }
namespace boken { class string_buffer_base; }

namespace boken {

class item : public object<item, item_definition, item_instance_id> {
public:
    using object::object;
};

item_pile const& items(const_item_descriptor i) noexcept;
item_pile&       items(item_descriptor       i) noexcept;

namespace detail {

bool impl_can_add_item(
    const_context                  ctx
  , const_entity_descriptor const* subject
  , const_item_descriptor          itm
  , const_item_descriptor          itm_dest
  , string_buffer_base&            result
) noexcept;

bool impl_can_remove_item(
    const_context           ctx
  , const_entity_descriptor subject
  , const_item_descriptor   itm_source
  , const_item_descriptor   itm
  , string_buffer_base&     result
) noexcept;

} // namespace detail

inline bool can_add_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , object_t<const_item_descriptor>    const itm
  , to_t<const_item_descriptor>        const itm_dest
  , string_buffer_base&                      result
) noexcept {
    const_entity_descriptor const s = subject;
    return detail::impl_can_add_item(ctx, &s, itm, itm_dest, result);
}

inline bool can_remove_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , from_t<const_item_descriptor>      const itm_source
  , object_t<const_item_descriptor>    const itm
  , string_buffer_base&                      result
) noexcept {
    return detail::impl_can_remove_item(ctx, subject, itm_source, itm, result);
}

void merge_into_pile(context ctx, unique_item itm_ptr, item_descriptor itm
    , item_descriptor dst);

//! get the item id to use for the display of item piles.
item_id get_pile_id(game_database const& db) noexcept;

//! get the item id to use to display an item pile
//! return an empty id when the id for generic piles should be used
item_id get_pile_id(const_context ctx, item_pile const& pile) noexcept;

} //namespace boken
