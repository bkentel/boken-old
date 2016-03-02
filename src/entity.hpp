#pragma once

#include "types.hpp"
#include "entity_def.hpp"
#include "object.hpp"

namespace boken { class game_database; }

namespace boken {

class entity
  : public object<entity_instance_id, entity_definition> {
public:
    using object::object;
};

} //namespace boken
