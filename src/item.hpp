#pragma once

#include "types.hpp"
#include "item_def.hpp"
#include "object.hpp"

#include <string>

namespace boken { class game_database; };

namespace boken {

class item : public object<item, item_definition, item_instance_id> {
public:
    using object::object;
};

//! Returns whether the item @p dest can have the item @p itm placed within it.
bool can_add_item(game_database const& db, item const& dest, item const& itm) noexcept;
bool can_add_item(game_database const& db, item const& dest, item_definition const& def) noexcept;

//! Get the weight of an item exclusive of any other items that might be
//! contained within it.
int32_t weight_of_exclusive(game_database const& db, item const& itm) noexcept;
int32_t weight_of_exclusive(world const& w, game_database const& db, item_instance_id id) noexcept;

//! Get the weight of an item inclusive of the weight of any other items that
//! might be contained within it.
int32_t weight_of_inclusive(world const& w, game_database const& db, item const& itm) noexcept;
int32_t weight_of_inclusive(world const& w, game_database const& db, item_instance_id id) noexcept;

//! get the item id to use for the display of item piles.
item_id get_pile_id(game_database const& db) noexcept;

//! get the item id to display an item pile
//! @pre @p pile is not empty
item_id get_pile_id(world const& w, item_pile const& pile, item_id pile_id) noexcept;

//! return a positive integer which is the capacity of the item, or 0 otherwise.
uint32_t is_container(game_database const& db, item const& itm) noexcept;
uint32_t is_container(item const& itm, item_definition const& def) noexcept;

//! return a positive integer indicating the identification state of the item
uint32_t is_identified(game_database const& db, item const& itm) noexcept;
uint32_t is_identified(item const& itm, item_definition const& def) noexcept;

//! return a detailed description of the item
//! @note This build a new description every time it is called; it is meant for
//!       generating descriptions to display to the player only.
//@{
std::string item_description(world const& w, game_database const& db, item const& itm, item_definition const& def);
std::string item_description(world const& w, game_database const& db, item_instance_id id);
//@}

std::string name_of_decorated(world const& w, game_database const& db, item const& itm, item_definition const& def);
std::string name_of_decorated(world const& w, game_database const& db, item const& itm);
std::string name_of_decorated(world const& w, game_database const& db, item_instance_id id);

} //namespace boken
