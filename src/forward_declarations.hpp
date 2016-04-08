#pragma once

#include "types.hpp"

namespace boken { struct entity_definition; }
namespace boken { struct item_definition; }
namespace boken { class entity; }
namespace boken { class item; }
namespace boken { class item_pile; }
namespace boken { class world; }
namespace boken { class game_database; }

namespace boken {

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

} //namespace boken
