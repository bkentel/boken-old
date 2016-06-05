#pragma once

#include "types.hpp"
#include "context_fwd.hpp"

#include <functional>

//=====--------------------------------------------------------------------=====
//                            Forward Declarations
//=====--------------------------------------------------------------------=====
namespace boken {
struct entity_definition;
struct item_definition;

class entity;
class item;

class item_pile;
class world;
class game_database;
class random_state;

} // namespace boken
//=====--------------------------------------------------------------------=====

namespace boken {

// object creation
unique_item create_object(world& w, std::function<item (item_instance_id)> const& f);
unique_entity create_object(world& w, std::function<entity (entity_instance_id)> const& f);

unique_item create_object(game_database const& db, world& w, item_definition const& def, random_state& rng);
unique_entity create_object(game_database const& db, world& w, entity_definition const& def, random_state& rng);

// object -> instance
entity_instance_id get_instance(entity const& e) noexcept;
entity_instance_id get_instance(const_entity_descriptor e) noexcept;
item_instance_id   get_instance(item const& i) noexcept;
item_instance_id   get_instance(const_item_descriptor i) noexcept;

item_pile&       get_items(item&         i) noexcept;
item_pile const& get_items(item const&   i) noexcept;
item_pile&       get_items(entity&       e) noexcept;
item_pile const& get_items(entity const& e) noexcept;

inline item_pile&       get_items(item_pile&       i) noexcept { return i; }
inline item_pile const& get_items(item_pile const& i) noexcept { return i; }

} //namespace boken
