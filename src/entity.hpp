#pragma once

#include "types.hpp"
#include "context.hpp"
#include "entity_def.hpp"
#include "object.hpp"

#include <cstdint>

namespace boken { class item; }
namespace boken { class random_state; }
namespace boken { class string_buffer_base; }

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

    // equip assumes that all prerequisites for an item are already met.
    // always check with can_equip_item first before calling this function.
    //@{
    void equip(item_instance_id id);
    void equip(body_part_id part_id, item_instance_id id);
    //@}

    // unequip assumes that all prerequisites for removing item are already met.
    // always check with can_unequip_item first before calling this function.
    void unequip(item_instance_id id);
private:
    std::reference_wrapper<item_deleter const> item_deleter_;
    std::vector<body_part> body_parts_;

    int16_t max_health_;
    int16_t cur_health_;
};

item_pile const& items(const_entity_descriptor e) noexcept;
item_pile&       items(entity_descriptor       e) noexcept;

namespace detail {

bool impl_can_add_item(
    const_context           ctx
  , const_entity_descriptor subject
  , const_item_descriptor   itm
  , const_entity_descriptor itm_dest
  , string_buffer_base&     result
) noexcept;

bool impl_can_remove_item(
    const_context           ctx
  , const_entity_descriptor subject
  , const_entity_descriptor itm_source
  , const_item_descriptor   itm
  , string_buffer_base&     result
) noexcept;

bool impl_can_equip_item(
    const_context           ctx
  , const_entity_descriptor subject
  , const_entity_descriptor itm_source
  , const_item_descriptor   itm
  , const_entity_descriptor itm_dest
  , body_part const*        part
  , string_buffer_base&     result
) noexcept;

bool impl_try_equip_item(
    const_context       ctx
  , entity_descriptor   subject
  , entity_descriptor   itm_source
  , item_descriptor     itm
  , entity_descriptor   itm_dest
  , body_part*          part
  , string_buffer_base& result
) noexcept;

bool impl_can_unequip_item(
    const_context           ctx
  , const_entity_descriptor subject
  , const_entity_descriptor itm_source
  , const_item_descriptor   itm
  , const_entity_descriptor itm_dest
  , body_part const*        part
  , string_buffer_base&     result
) noexcept;

bool impl_try_unequip_item(
    const_context       ctx
  , entity_descriptor   subject
  , entity_descriptor   itm_source
  , item_descriptor     itm
  , entity_descriptor   itm_dest
  , body_part*          part
  , string_buffer_base& result
) noexcept;

} // namespace detail

//! returns whether the subject can equip the given item. If the subject can't,
//! a formatted reason is written to the buffer given by result
inline bool can_equip_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , from_t<const_entity_descriptor>    const itm_source
  , object_t<const_item_descriptor>    const itm
  , to_t<const_entity_descriptor>      const itm_dest
  , body_part const&                         part
  , string_buffer_base&                      result
) noexcept {
    return detail::impl_can_equip_item(
        ctx, subject, itm_source, itm, itm_dest, &part, result);
}

inline bool can_equip_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , from_t<const_entity_descriptor>    const itm_source
  , object_t<const_item_descriptor>    const itm
  , to_t<const_entity_descriptor>      const itm_dest
  , string_buffer_base&                      result
) noexcept {
    return detail::impl_can_equip_item(
        ctx, subject, itm_source, itm, itm_dest, nullptr, result);
}

inline bool try_equip_item(
    const_context                const ctx
  , subject_t<entity_descriptor> const subject
  , from_t<entity_descriptor>    const itm_source
  , object_t<item_descriptor>    const itm
  , to_t<entity_descriptor>      const itm_dest
  , body_part&                         part
  , string_buffer_base&                result
) noexcept {
    return detail::impl_try_equip_item(
        ctx, subject, itm_source, itm, itm_dest, &part, result);
}

inline bool try_equip_item(
    const_context                const ctx
  , subject_t<entity_descriptor> const subject
  , from_t<entity_descriptor>    const itm_source
  , object_t<item_descriptor>    const itm
  , to_t<entity_descriptor>      const itm_dest
  , string_buffer_base&                result
) noexcept {
    return detail::impl_try_equip_item(
        ctx, subject, itm_source, itm, itm_dest, nullptr, result);
}

//! returns whether the subject can unequip the given item. If the subject can't,
//! a formatted reason is written to the buffer given by result
inline bool can_unequip_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , from_t<const_entity_descriptor>    const itm_source
  , object_t<const_item_descriptor>    const itm
  , to_t<const_entity_descriptor>      const itm_dest
  , body_part const&                         part
  , string_buffer_base&                      result
) noexcept {
    return detail::impl_can_unequip_item(
        ctx, subject, itm_source, itm, itm_dest, &part, result);
}

inline bool can_unequip_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , from_t<const_entity_descriptor>    const itm_source
  , object_t<const_item_descriptor>    const itm
  , to_t<const_entity_descriptor>      const itm_dest
  , string_buffer_base&                      result
) noexcept {
    return detail::impl_can_unequip_item(
        ctx, subject, itm_source, itm, itm_dest, nullptr, result);
}

inline bool try_unequip_item(
    const_context                const ctx
  , subject_t<entity_descriptor> const subject
  , from_t<entity_descriptor>    const itm_source
  , object_t<item_descriptor>    const itm
  , to_t<entity_descriptor>      const itm_dest
  , body_part&                         part
  , string_buffer_base&                result
) noexcept {
    return detail::impl_try_unequip_item(
        ctx, subject, itm_source, itm, itm_dest, &part, result);
}

inline bool try_unequip_item(
    const_context                const ctx
  , subject_t<entity_descriptor> const subject
  , from_t<entity_descriptor>    const itm_source
  , object_t<item_descriptor>    const itm
  , to_t<entity_descriptor>      const itm_dest
  , string_buffer_base&                result
) noexcept {
    return detail::impl_try_unequip_item(
        ctx, subject, itm_source, itm, itm_dest, nullptr, result);
}

inline bool can_add_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , object_t<const_item_descriptor>    const itm
  , to_t<const_entity_descriptor>      const itm_dest
  , string_buffer_base&                      result
) noexcept {
    return detail::impl_can_add_item(ctx, subject, itm, itm_dest, result);
}

inline bool can_remove_item(
    const_context                      const ctx
  , subject_t<const_entity_descriptor> const subject
  , from_t<const_entity_descriptor>    const itm_source
  , object_t<const_item_descriptor>    const itm
  , string_buffer_base&                      result
) noexcept {
    return detail::impl_can_remove_item(ctx, subject, itm_source, itm, result);
}

void merge_into_pile(context ctx, unique_item itm_ptr, item_descriptor itm
    , entity_descriptor dst);

} //namespace boken
