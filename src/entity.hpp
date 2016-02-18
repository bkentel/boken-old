#pragma once

#include "types.hpp"

namespace boken {

class entity {
public:
    entity(entity_instance_id const e_instance, entity_id const e_id)
      : instance_id {e_instance}
      , id {e_id}
    {
    }

    entity_instance_id instance_id {0};
    entity_id          id          {0};
};

} //namespace boken
