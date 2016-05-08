#pragma once

#include "config.hpp"
#include "context.hpp"
#include <string>

namespace boken { class string_buffer_base; }

namespace boken {
namespace msg {

using ced = const_entity_descriptor;
using cid = const_item_descriptor;
using cll = const_level_location;

//! The message for (un)successfully moving an item from an entity->level.
std::string drop_item(
    const_context ctx, ced sub, cid obj, ced src, cll dst, string_view fail);

namespace detail {

//! The message for (un)successfully moving an item from an item->level.
std::string remove_item(
    const_context ctx, ced sub, cid obj, cid src, cll dst, string_view fail);

//! The message for (un)successfully moving an item from an item->entity.
std::string remove_item(
    const_context ctx, ced sub, cid obj, cid src, ced dst, string_view fail);

} // namespace detail

template <typename To>
std::string remove_item(
    const_context ctx, ced sub, cid obj, cid src, To dst, string_view fail
) {
    return detail::remove_item(ctx, sub, obj, src, dst, fail);
}

//! The message for (un)successfully moving an item from an entity->item.
std::string insert_item(
    const_context ctx, ced sub, cid obj, ced src, cid dst, string_view fail);

//! The message for (un)successfully moving an item from an level->entity.
std::string pickup_item(
    const_context ctx, ced sub, cid obj, cll src, ced dst, string_view fail);

//! The string displayed for object debug messages
bool debug_item_info(string_buffer_base& buffer, const_context ctx, cid itm);
bool debug_entity_info(string_buffer_base& buffer, const_context ctx, ced ent);

std::string debug_item_info(const_context ctx, cid itm);
std::string debug_entity_info(const_context ctx, ced ent);

//! The string diplayed by the 'view' command for objects.
bool view_item_info(string_buffer_base& buffer, const_context ctx, cid itm);
bool view_entity_info(string_buffer_base buffer, const_context ctx, ced ent);

std::string view_item_info(const_context ctx, cid itm);
std::string view_entity_info(const_context ctx, ced ent);

} // namespace msg
} // namespace boken
