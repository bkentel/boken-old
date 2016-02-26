#pragma once

#include "definition.hpp"
#include "types.hpp"

namespace boken {

struct entity_definition : basic_definition {
    entity_definition(entity_id const id_ = entity_id {0})
      : basic_definition {"{null}", "{null}", "{null}", 0}
      , id {id_}
      , tile_index {0}
    {
    }

    entity_id id;
    uint16_t  tile_index;
};

} //namespace boken
