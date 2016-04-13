#include "entity.hpp"
#include "forward_declarations.hpp"
#include "math.hpp"

namespace boken {

//=====--------------------------------------------------------------------=====
//                               free functions
//=====--------------------------------------------------------------------=====
entity_instance_id get_instance(entity const& e) noexcept {
    return e.instance();
}

entity_id get_id(entity const& e) noexcept {
    return e.definition();
}

item_pile& get_items(entity& e) noexcept {
    return e.items();
}

item_pile const& get_items(entity const& e) noexcept {
    return e.items();
}

//=====--------------------------------------------------------------------=====
//                                  entity
//=====--------------------------------------------------------------------=====
entity::entity(entity_instance_id const instance, entity_id const id) noexcept
  : object {instance, id}
  , max_health_ {1}
  , cur_health_ {1}
{
}

bool entity::is_alive() const noexcept {
    return cur_health_ > 0;
}

bool entity::modify_health(int16_t const delta) noexcept {
    constexpr int32_t lo = std::numeric_limits<int16_t>::min();
    constexpr int32_t hi = std::numeric_limits<int16_t>::max();

    auto const sum = int32_t {delta} + int32_t {cur_health_};

    cur_health_ = clamp_as<int16_t>(sum, lo, hi);

    return is_alive();
}

bool entity::can_add_item(item const&) const noexcept {
    return true;
}

} //namespace boken
