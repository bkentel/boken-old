#pragma once

#include "types.hpp"
#include "item_def.hpp"
#include "object.hpp"

namespace boken { class world; }

namespace boken {

class item
  : public object<item_instance_id, item_definition> {
public:
    using object::object;
};

item const* find(world const& w, item_instance_id id) noexcept;

} //namespace boken
