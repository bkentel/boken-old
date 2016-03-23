#pragma once

#include "types.hpp"
#include "forward_declarations.hpp"
#include "item_def.hpp"
#include "object.hpp"

namespace boken {

class item
  : public object<item_instance_id, item_definition> {
public:
    using object::object;
};

} //namespace boken
