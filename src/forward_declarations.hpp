// This header exists primarily to break dependencies on full class definitions.

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

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                         object lookup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
item   const& find(world const& w, item_instance_id   id) noexcept;
entity const& find(world const& w, entity_instance_id id) noexcept;

item&   find(world& w, item_instance_id   id) noexcept;
entity& find(world& w, entity_instance_id id) noexcept;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                         object definition lookup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// id -> definition
item_definition const* find(game_database const& db, item_id id) noexcept;
entity_definition const* find(game_database const& db, entity_id id) noexcept;

// contained items
item_pile& get_items(item& i) noexcept;
item_pile const& get_items(item const& i) noexcept;

item_pile& get_items(entity& e) noexcept;
item_pile const& get_items(entity const& e) noexcept;

inline item_pile& get_items(item_pile& i) noexcept { return i; }
inline item_pile const& get_items(item_pile const& i) noexcept { return i; }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                              object properties
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//------------------------------------------------------------------------------

entity_property_value get_property_value_or(
    entity const&            itm
  , entity_definition const& def
  , entity_property_id       property
  , entity_property_value    fallback) noexcept;

entity_property_value get_property_value_or(
    game_database const&  db
  , entity const&         itm
  , entity_property_id    property
  , entity_property_value fallback) noexcept;

entity_property_value get_property_value_or(
    game_database const&  db
  , entity_id             id
  , entity_property_id    property
  , entity_property_value fallback) noexcept;

entity_property_value get_property_value_or(
    world const&          w
  , game_database const&  db
  , entity_instance_id    id
  , entity_property_id    property
  , entity_property_value fallback) noexcept;

//------------------------------------------------------------------------------

item_property_value get_property_value_or(
    item const&            itm
  , item_definition const& def
  , item_property_id       property
  , item_property_value    fallback) noexcept;

item_property_value get_property_value_or(
    game_database const& db
  , item const&          itm
  , item_property_id     property
  , item_property_value  fallback) noexcept;

item_property_value get_property_value_or(
    game_database const& db
  , item_id              id
  , item_property_id     property
  , item_property_value  fallback) noexcept;

item_property_value get_property_value_or(
    world const&         w
  , game_database const& db
  , item_instance_id     id
  , item_property_id     property
  , item_property_value  fallback) noexcept;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                            object names
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

string_view name_of(game_database const& db, item_id id) noexcept;
string_view name_of(game_database const& db, item const& i) noexcept;
string_view name_of(world const& w, game_database const& db, item_instance_id id) noexcept;

string_view name_of(game_database const& db, entity_id id) noexcept;
string_view name_of(game_database const& db, entity const& e) noexcept;
string_view name_of(world const& w, game_database const& db, entity_instance_id id) noexcept;

} //namespace boken
