#pragma once

#include "types.hpp"

namespace boken { class entity; }
namespace boken { class item; }
namespace boken { class world; }

namespace boken {

entity_instance_id get_instance(entity const& e) noexcept;
item_instance_id get_instance(item const& i) noexcept;

entity_id get_id(entity const& e) noexcept;
item_id get_id(item const& i) noexcept;

item const* find(world const& w, item_instance_id id) noexcept;
entity const* find(world const& w, entity_instance_id id) noexcept;

item* find(world& w, item_instance_id id) noexcept;
entity* find(world& w, entity_instance_id id) noexcept;

} //namespace boken
