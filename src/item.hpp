#pragma once

#include "types.hpp"
#include "item_def.hpp"
#include "object.hpp"

namespace boken {

class item : public object<item, item_definition, item_instance_id> {
public:
    using object::object;

    bool can_add_item(item const&) const noexcept {
        return true;
    }
};

} //namespace boken
