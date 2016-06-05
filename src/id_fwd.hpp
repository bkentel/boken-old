#pragma once

#include <cstdint>

namespace boken {

template <typename, typename> class tagged_value;

struct tag_id_entity;
struct tag_id_instance_entity;
struct tag_id_property_entity;
struct tag_id_item;
struct tag_id_instance_item;
struct tag_id_property_item;
struct tag_id_region;
struct tag_id_body_part;

using entity_id          = tagged_value<uint32_t, tag_id_entity>;
using entity_instance_id = tagged_value<uint32_t, tag_id_instance_entity>;
using entity_property_id = tagged_value<uint32_t, tag_id_property_entity>;
using item_id            = tagged_value<uint32_t, tag_id_item>;
using item_instance_id   = tagged_value<uint32_t, tag_id_instance_item>;
using item_property_id   = tagged_value<uint32_t, tag_id_property_item>;
using body_part_id       = tagged_value<uint32_t, tag_id_body_part>;

} // namespace boken
