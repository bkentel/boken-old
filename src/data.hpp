#pragma once

#include "types.hpp"
#include <memory>

namespace boken {

struct item_definition;
struct entity_definition;

//====---
// The database of all current game data.
//====---
class game_database {
public:
    virtual item_definition   const* find(item_id   id) const noexcept = 0;
    virtual entity_definition const* find(entity_id id) const noexcept = 0;
};

std::unique_ptr<game_database> make_game_database();

} //namespace boken
