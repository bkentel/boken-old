#include "entity.hpp"
#include "algorithm.hpp"
#include "entity_properties.hpp"
#include "item_properties.hpp"
#include "forward_declarations.hpp"
#include "math.hpp"

namespace boken {

//=====--------------------------------------------------------------------=====
//                               free functions
//=====--------------------------------------------------------------------=====

item_pile const& items(const_entity_descriptor const e) noexcept {
    return e.obj.items();
}

item_pile& items(entity_descriptor const e) noexcept {
    return e.obj.items();
}

string_view id_string(const_entity_descriptor const e) noexcept {
    return e.def
      ? string_view {e.def->id_string.data()}
      : string_view {"{missing definition}"};
}

entity_id get_id(entity_definition const& def) noexcept {
    return def.id;
}

namespace detail {

string_view impl_can_add_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
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
  , const_entity_descriptor const subject
  , point2i32               const subject_p
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
    game_database      const& db
  , world              const& w
  , entity_instance_id const  instance
  , entity_definition  const& def
  , random_state&             rng
) {
    return {get_item_deleter(w), db, def, instance, rng};
}

} // namespace

unique_entity create_object(
    game_database const&     db
  , world&                   w
  , entity_definition const& def
  , random_state&            rng
) {
    return create_object(w, [&](entity_instance_id const instance) {
        return create_object(db, w, instance, def, rng);
    });
}

bool can_equip_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
  , const_item_descriptor   const itm
  , string_buffer_base*     const result
) noexcept {
    if (!itm.def) {
        return result
            && result->append("{missing definition for item}")
            && false;
    }

    if (!subject) {
        return result
            && result->append("{missing definition for entity}")
            && false;
    }

    auto const item_can_equip =
        get_property_value_or(itm, property(item_property::can_equip), 0);

    if (!item_can_equip) {
        return result
            && result->append("the %s can't be equipped"
                 , name_of_decorated(ctx, itm).data())
            && false;
    }

    auto const subject_can_equip =
        get_property_value_or(subject, property(entity_property::can_equip), 0);

    if (!subject_can_equip) {
        return result
            && result->append("%s can't equip any items"
                 , name_of_decorated(ctx, subject).data())
            && false;
    }

    auto const first = subject.obj.body_begin();
    auto const last  = subject.obj.body_end();
    auto const it = std::find_if(first, last, [&](body_part const& part) {
        return part.is_free();
    });

    if (it == last) {
        return result
            && result->append("%s has no free equipment slots"
                 , name_of_decorated(ctx, subject).data())
            && false;
    }

    return true;
}

//=====--------------------------------------------------------------------=====
//                                  entity
//=====--------------------------------------------------------------------=====
entity::~entity() {
    for (auto const& part : body_parts_) {
        unique_item {part.equip, item_deleter_};
    }
}

entity::entity(
    item_deleter       const& deleter
  , game_database      const& db
  , entity_definition  const& def
  , entity_instance_id const  instance
  , random_state&             rng
) noexcept
  : object {deleter, instance, def.id}
  , item_deleter_ {deleter}
  , max_health_ {1}
  , cur_health_ {1}
{
    auto const n = def.properties.value_or(
        entity_property_id {djb2_hash_32c("body_n")}, 0);

    if (n <= 0) {
        return;
    }

    body_parts_.reserve(static_cast<size_t>(n));

    std::generate_n(back_inserter(body_parts_), n, [&, i = 0]() mutable {
        char key[] = "body_\0\0";
        if (n <= 9) {
            *(std::end(key) - 3) = '0' + static_cast<char>(i);
        } else {
            BK_ASSERT(n <= 99);
            *(std::end(key) - 2) = '0' + static_cast<char>(i / 10);
            *(std::end(key) - 3) = '0' + static_cast<char>(i % 10);
        }

        ++i;

        auto const id = def.properties.value_or(
            entity_property_id {djb2_hash_32(key)}, 0);

        BK_ASSERT(id != 0);

        return body_part {id, item_instance_id {}};
    });
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

body_part const* entity::body_begin() const noexcept {
    return body_parts_.data();
}

body_part const* entity::body_end() const noexcept {
    return body_parts_.data() + body_parts_.size();
}

void entity::equip(int32_t const index) {
    BK_ASSERT(check_index(index, items().size()));
    equip(items()[static_cast<size_t>(index)]);
}

void entity::equip(item_instance_id const id) {
    auto const last = std::end(body_parts_);
    auto const it = std::find_if(begin(body_parts_), last
      , [&](body_part const& part) { return part.is_free(); });

    BK_ASSERT(it != last
           && it->equip == item_instance_id {});

    auto itm = items().remove_item(id);
    BK_ASSERT(!!itm);

    it->equip = itm.release();
}

} //namespace boken
