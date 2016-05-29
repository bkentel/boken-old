#pragma once

#include "types.hpp"
#include "context.hpp"
#include "entity_def.hpp"
#include "object.hpp"
#include "format.hpp"

#include <cstdint>

namespace boken { class item; }

namespace boken {

struct body_part {
    bool is_free() const noexcept {
        return equip == item_instance_id {};
    }

    body_part_id     id;
    item_instance_id equip;
};

class entity : public object<entity, entity_definition, entity_instance_id> {
public:
    ~entity();

    entity(
        item_deleter const& deleter
      , game_database const& db
      , entity_definition const& def
      , entity_instance_id instance
      , random_state& rng
    ) noexcept;

    entity(entity const&) = delete;
    entity& operator=(entity const&) = delete;

    entity(entity&&) = default;
    entity& operator=(entity&&) = default;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // stats
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bool is_alive() const noexcept;
    bool modify_health(int16_t delta) noexcept;

    body_part const* body_begin() const noexcept;
    body_part const* body_end() const noexcept;

    void equip(int32_t index);
    void equip(item_instance_id id);
private:
    std::reference_wrapper<item_deleter const> item_deleter_;
    std::vector<body_part> body_parts_;

    int16_t max_health_;
    int16_t cur_health_;
};

item_pile const& items(const_entity_descriptor e) noexcept;
item_pile&       items(entity_descriptor       e) noexcept;

namespace detail {

string_view impl_can_add_item(const_context ctx
    , const_entity_descriptor subject, point2i32 subject_p
    , const_item_descriptor itm, const_entity_descriptor dst) noexcept;

string_view impl_can_remove_item(const_context ctx
    , const_entity_descriptor subject, point2i32 subject_p
    , const_item_descriptor itm, const_entity_descriptor src) noexcept;

} // namespace detail

//! returns whether the subject can equip the given item. If the subject can't,
//! a formatted reason is written to the buffer given by result
//@{
bool can_equip_item(
    const_context           ctx
  , const_entity_descriptor subject
  , const_item_descriptor   itm
  , string_buffer_base&     result
) noexcept;

bool can_equip_item(
    const_context           ctx
  , const_entity_descriptor subject
  , const_item_descriptor   itm
) noexcept;
//@}

template <typename UnaryF>
bool can_add_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
  , const_item_descriptor   const itm
  , const_entity_descriptor const dst
  , UnaryF                  const on_fail
) noexcept {
    return not_empty_or(on_fail
      , detail::impl_can_add_item(ctx, subject, subject_p, itm, dst));
}

template <typename UnaryF>
bool can_remove_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
  , const_item_descriptor   const itm
  , const_entity_descriptor const src
  , UnaryF                  const on_fail
) noexcept {
    return not_empty_or(on_fail
      , detail::impl_can_remove_item(ctx, subject, subject_p, itm, src));
}

void merge_into_pile(context ctx, unique_item itm_ptr, item_descriptor itm
    , entity_descriptor dst);

} //namespace boken
