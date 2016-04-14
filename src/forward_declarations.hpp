#pragma once

#include "types.hpp"
#include <functional>

namespace boken { struct entity_definition; }
namespace boken { struct item_definition; }
namespace boken { class entity; }
namespace boken { class item; }
namespace boken { class item_pile; }
namespace boken { class world; }
namespace boken { class game_database; }
namespace boken { class random_state; }

namespace boken {

// object creation
unique_item create_object(world& w, std::function<item (item_instance_id)> const& f);
unique_entity create_object(world& w, std::function<entity (entity_instance_id)> const& f);

unique_item create_object(world& w, item_definition const& def, random_state& rng);
unique_entity create_object(world& w, entity_definition const& def, random_state& rng);

// object -> instance
entity_instance_id get_instance(entity const& e) noexcept;
item_instance_id get_instance(item const& i) noexcept;

// object -> id
entity_id get_id(entity const& e) noexcept;
item_id get_id(item const& i) noexcept;

// instance -> object
item const* find(world const& w, item_instance_id id) noexcept;
entity const* find(world const& w, entity_instance_id id) noexcept;

item* find(world& w, item_instance_id id) noexcept;
entity* find(world& w, entity_instance_id id) noexcept;

// id -> definition
item_definition const* find(game_database const& db, item_id id) noexcept;
entity_definition const* find(game_database const& db, entity_id id) noexcept;

// contained items
item_pile* get_items(item& i) noexcept;
item_pile const* get_items(item const& i) noexcept;

item_pile& get_items(entity& e) noexcept;
item_pile const& get_items(entity const& e) noexcept;

template <typename DefId, typename PropertyId>
bool has_property(game_database const& data, DefId id, PropertyId property) noexcept;

template <typename Definition, typename PropertyId>
bool has_property(Definition const& def, PropertyId property) noexcept;

template <typename DefId, typename PropertyId, typename PropertyValue>
PropertyValue property_value_or(game_database const& data, DefId id, PropertyId property, PropertyValue value) noexcept;

template <typename Definition, typename PropertyId, typename PropertyValue>
PropertyValue property_value_or(Definition const& def, PropertyId property, PropertyValue value) noexcept;

} //namespace boken
