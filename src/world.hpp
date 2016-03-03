#pragma once

#include "types.hpp"
#include <memory>
#include <functional>

namespace boken {

class item;
class entity;
class level;

//====---
// (Singular) World state.
//====---
class world {
public:
    virtual ~world();

    virtual item   const* find(item_instance_id   id) const noexcept = 0;
    virtual entity const* find(entity_instance_id id) const noexcept = 0;

    virtual unique_item create_item(std::function<item (item_instance_id)> f) = 0;

    virtual entity_instance_id create_entity_id() = 0;

    virtual void free_item(item_instance_id id) = 0;
    virtual void free_entity(entity_instance_id id) = 0;

    virtual int total_levels() const noexcept = 0;

    virtual level&       current_level()       noexcept = 0;
    virtual level const& current_level() const noexcept = 0;

    virtual level& add_new_level(level* parent, std::unique_ptr<level> level) = 0;
};

std::unique_ptr<world> make_world();

} //namespace boken
