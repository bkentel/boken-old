#pragma once

#include "types.hpp"
#include "entity_def.hpp"
#include "object.hpp"
#include <cstdint>

namespace boken { class item; }

namespace boken {

class entity : public object<entity, entity_definition, entity_instance_id> {
public:
    entity(entity_instance_id instance, entity_id id) noexcept;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // stats
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bool is_alive() const noexcept;
    bool modify_health(int16_t delta) noexcept;
private:
    int16_t max_health_;
    int16_t cur_health_;
};

bool can_add_item(game_database const& db, entity const& dest, item const& itm) noexcept;
bool can_add_item(game_database const& db, entity const& dest, item_definition const& def) noexcept;

} //namespace boken
