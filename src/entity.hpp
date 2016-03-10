#pragma once

#include "types.hpp"
#include "entity_def.hpp"
#include "object.hpp"
#include "item_pile.hpp"

namespace boken { class item; }
namespace boken { class world; }

namespace boken {

class entity
  : public object<entity_instance_id, entity_definition> {
public:
    using object::object;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // stats
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bool is_alive() const noexcept;
    bool modify_health(int16_t delta) noexcept;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // item / inventory management
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bool can_add_item(item const& itm);
    void add_item(unique_item itm);
private:
    int16_t max_health_;
    int16_t cur_health_;
    item_pile items_;
};

entity const* find(world const& w, entity_instance_id id) noexcept;

} //namespace boken
