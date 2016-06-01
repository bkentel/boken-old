#pragma once

namespace boken {

class entity;
class item;

struct entity_definition;
struct item_definition;

template <typename, typename> struct descriptor_base;
template <bool>               struct context_base;
template <bool>               struct level_location_base;

using context       = context_base<false>;
using const_context = context_base<true>;

using level_location       = level_location_base<false>;
using const_level_location = level_location_base<true>;

using item_descriptor       = descriptor_base<item,       item_definition>;
using const_item_descriptor = descriptor_base<item const, item_definition>;

using entity_descriptor       = descriptor_base<entity,       entity_definition>;
using const_entity_descriptor = descriptor_base<entity const, entity_definition>;

} // namespace boken
