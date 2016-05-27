// This header exists primarily to break dependencies on full class definitions.

#pragma once

#include "types.hpp"
#include "context.hpp"

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

unique_item create_object(game_database const& db, world& w, item_definition const& def, random_state& rng);
unique_entity create_object(game_database const& db, world& w, entity_definition const& def, random_state& rng);

// object -> instance
entity_instance_id get_instance(entity const& e) noexcept;
item_instance_id   get_instance(item   const& i) noexcept;

item_pile&       get_items(item&         i) noexcept;
item_pile const& get_items(item const&   i) noexcept;
item_pile&       get_items(entity&       e) noexcept;
item_pile const& get_items(entity const& e) noexcept;

inline item_pile&       get_items(item_pile&       i) noexcept { return i; }
inline item_pile const& get_items(item_pile const& i) noexcept { return i; }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                              object properties
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

entity_property_value get_property_value_or(
    const_entity_descriptor ent
  , entity_property_id      property
  , entity_property_value   fallback) noexcept;

item_property_value get_property_value_or(
    const_item_descriptor itm
  , item_property_id      property
  , item_property_value   fallback) noexcept;

} //namespace boken
