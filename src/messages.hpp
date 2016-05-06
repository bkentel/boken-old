#pragma once

#include "config.hpp"
#include "context.hpp"
#include <string>

namespace boken {
namespace msg {

using ced = const_entity_descriptor;
using cid = const_item_descriptor;
using cll = const_level_location;

std::string drop_item(
    const_context ctx, ced sub, cid obj, ced src, cll dst, string_view fail);

namespace detail {

std::string remove_item(
    const_context ctx, ced sub, cid obj, cid src, cll dst, string_view fail);

std::string remove_item(
    const_context ctx, ced sub, cid obj, cid src, ced dst, string_view fail);

} // namespace detail

template <typename To>
std::string remove_item(
    const_context ctx, ced sub, cid obj, cid src, To dst, string_view fail
) {
    return detail::remove_item(ctx, sub, obj, src, dst, fail);
}

std::string insert_item(
    const_context ctx, ced sub, cid obj, ced src, cid dst, string_view fail);

std::string pickup_item(
    const_context ctx, ced sub, cid obj, cll src, ced dst, string_view fail);

} // namespace msg
} // namespace boken
