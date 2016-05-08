#include "entity.hpp"
#include "entity_properties.hpp"
#include "forward_declarations.hpp"
#include "math.hpp"

namespace boken {

//=====--------------------------------------------------------------------=====
//                               free functions
//=====--------------------------------------------------------------------=====

entity_id get_id(entity_definition const& def) noexcept {
    return def.id;
}

namespace detail {

string_view impl_can_add_item(
    const_context           const ctx
  , const_item_descriptor   const itm
  , const_entity_descriptor const dest
) noexcept {
    if (!itm.def) {
        return "{missing definition for item}";
    }

    if (!dest) {
        return "{missing definition for destination entity}";
    }

    return {};
}

string_view impl_can_remove_item(
    const_context           const ctx
  , const_item_descriptor   const itm
  , const_entity_descriptor const dest
) noexcept {
    return {};
}

} // namespace detail

void merge_into_pile(
    context           const ctx
  , unique_item             itm_ptr
  , item_descriptor   const itm
  , entity_descriptor const pile
) {
    merge_into_pile(ctx, std::move(itm_ptr), itm, pile.obj.items());
}

std::string name_of_decorated(
    const_context           const ctx
  , const_entity_descriptor const e
) {
    if (!e) {
        return "{missing definition}";
    }

    constexpr auto p_is_player = property(entity_property::is_player);

    if (get_property_value_or(e, p_is_player, 0)) {
        return "you";
    }

    return e.def->name;
}

string_view name_of(const_context const ctx, const_entity_descriptor const e) noexcept {
    return e
      ? string_view {e.def->name}
      : string_view {"{missing definition}"};
}

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

entity_property_value get_property_value_or(
    const_entity_descriptor const ent
  , entity_property_id      const property
  , entity_property_value   const fallback
) noexcept {
    return ent
      ? ent.obj.property_value_or(*ent.def, property, fallback)
      : fallback;
}

namespace {

entity create_object(
    entity_instance_id const  instance
  , entity_definition  const& def
  , random_state&             rng
) {
    entity result {instance, def.id};
    return result;
}

} // namespace

unique_entity create_object(
    world&                   w
  , entity_definition const& def
  , random_state&            rng
) {
    return create_object(w, [&](entity_instance_id const instance) {
        return create_object(instance, def, rng);
    });
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

} //namespace boken
