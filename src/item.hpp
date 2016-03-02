#pragma once

#include "types.hpp"
#include "item_def.hpp"

namespace boken { class game_database; }

namespace boken {

class item
  : public object<item_instance_id, item_definition> {
public:
    using object::object;
};

} //namespace boken
