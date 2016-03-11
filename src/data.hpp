#pragma once

#include "types.hpp"
#include <memory>

namespace boken { struct item_definition; }
namespace boken { struct entity_definition; }
namespace boken { class tile_map; }
namespace boken { enum class tile_map_type : uint32_t; }

namespace boken {

//=====--------------------------------------------------------------------=====
// The database of all current game data.
//=====--------------------------------------------------------------------=====
class game_database {
public:
    virtual ~game_database();

    //! @returns The definition associated with a given id. Otherwise, a nullptr
    //! if no such definition exists.
    virtual item_definition const* find(item_id id) const noexcept = 0;
    virtual entity_definition const* find(entity_id id) const noexcept = 0;

    virtual tile_map const& get_tile_map(tile_map_type type) const noexcept = 0;
};

std::unique_ptr<game_database> make_game_database();

} //namespace boken
