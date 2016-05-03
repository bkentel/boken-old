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

void merge_into_pile(
    context           ctx
  , unique_item       itm_ptr
  , item_descriptor   itm
  , entity_descriptor pile);

namespace detail {

string_view impl_can_add_item(context ctx, const_item_descriptor itm
                            , const_entity_descriptor dest) noexcept;

string_view impl_can_remove_item(context ctx, const_item_descriptor itm
                               , const_entity_descriptor dest) noexcept;

} // namespace detail

template <typename UnaryF>
bool can_add_item(
    context                 const ctx
  , const_item_descriptor   const itm
  , const_entity_descriptor const dest
  , UnaryF                        on_fail
) noexcept {
    auto const result = detail::impl_can_add_item(ctx, itm, dest);
    return result.empty()
      ? true
      : (on_fail(result), false);
}

template <typename UnaryF>
bool can_remove_item(
    context                 const ctx
  , const_item_descriptor   const itm
  , const_entity_descriptor const dest
  , UnaryF                        on_fail
) noexcept {
    auto const result = detail::impl_can_remove_item(ctx, itm, dest);
    return result.empty()
      ? true
      : (on_fail(result), false);
}

std::string name_of_decorated(context ctx, const_entity_descriptor e);

} //namespace boken
