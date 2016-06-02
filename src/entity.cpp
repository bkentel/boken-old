#include "entity.hpp"
#include "algorithm.hpp"
#include "entity_properties.hpp"
#include "item_properties.hpp"
#include "forward_declarations.hpp"
#include "math.hpp"
#include "format.hpp"
#include "hash.hpp"
#include "names.hpp"

namespace boken {

//! common entity properties
enum class entity_property : uint32_t {
    is_player = djb2_hash_32c("is_player")
  , can_equip = djb2_hash_32c("can_equip")
  , body_n    = djb2_hash_32c("body_n")
};

namespace {

constexpr entity_property_id property(entity_property const p) noexcept {
    using type = std::underlying_type_t<entity_property>;
    return entity_property_id {static_cast<type>(p)};
}

} // namespace

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

entity_id get_id(const_entity_descriptor const e) noexcept {
    return e->definition();
}

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

entity_instance_id get_instance(const_entity_descriptor const e) noexcept {
    return e->instance();
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

bool can_equip(const_entity_descriptor const e) noexcept {
    constexpr auto p_can_equip = property(entity_property::can_equip);
    return !!get_property_value_or(e, p_can_equip, 0);
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

namespace detail {

bool impl_can_add_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , const_item_descriptor   const itm
  , const_entity_descriptor const itm_dest
  , string_buffer_base&           result
) noexcept {
    if (!check_definitions(result, subject, itm, itm_dest)) {
        return false;
    }

    return true;
}

bool impl_can_remove_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , const_entity_descriptor const itm_source
  , const_item_descriptor   const itm
  , string_buffer_base&           result
) noexcept {
    if (!check_definitions(result, itm_source, itm)) {
        return false;
    }

    return true;
}

bool impl_can_equip_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , const_entity_descriptor const itm_source
  , const_item_descriptor   const itm
  , const_entity_descriptor const itm_dest
  , body_part const*        const part
  , string_buffer_base&           result
) noexcept {
    if (!check_definitions(result, subject, itm_source, itm, itm_dest)) {
        return false;
    }

    if (subject != itm_dest) {
        return result.append("%s can't equip the %s to %s"
          , name_of_decorated(ctx, subject).data()
          , name_of_decorated(ctx, itm).data()
          , name_of_decorated(ctx, itm_dest).data()), false;
    }

    if (subject != itm_source) {
        return result.append("%s can't equip the %s from %s"
          , name_of_decorated(ctx, subject).data()
          , name_of_decorated(ctx, itm).data()
          , name_of_decorated(ctx, itm_source).data()), false;
    }

    if (!can_equip(itm)) {
        return result.append("the %s can't be equipped"
            , name_of_decorated(ctx, itm).data()), false;
    }

    if (!can_equip(itm_dest)) {
        return result.append("%s can't equip any items"
            , name_of_decorated(ctx, itm_dest).data()), false;
    }

    auto const last  = itm_dest->body_end();
    auto const it = std::find_if(itm_dest->body_begin(), last
      , [&](body_part const& p) { return p.is_free(); });

    if (it == last) {
        return result.append("%s has no free equipment slots"
            , name_of_decorated(ctx, itm_dest).data()), false;
    }

    return true;
}

bool impl_try_equip_item(
    const_context       const ctx
  , entity_descriptor   const subject
  , entity_descriptor   const itm_source
  , item_descriptor     const itm
  , entity_descriptor   const itm_dest
  , body_part*          const part
  , string_buffer_base&       result
) noexcept {
    if (!impl_can_equip_item(ctx, subject, itm_source, itm, itm_dest, part, result)) {
        return false;
    }

    if (subject != itm_dest) {
        BK_ASSERT(false && "TODO");
    }

    if (subject != itm_source) {
        BK_ASSERT(false && "TODO");
    }

    auto const first = itm_dest->body_begin();
    auto const last  = itm_dest->body_end();

    auto const it = part
      ? std::find_if(first, last, [&](body_part const& p) { return p.id == part->id; })
      : std::find_if(first, last, [&](body_part const& p) { return p.is_free(); });

    BK_ASSERT((it != last)
           && (!part || std::addressof(*it) == part)
           && it->is_free());

    itm_dest->equip(it->id, get_instance(itm.obj));

    result.append("%s equip the %s to its %s."
      , name_of_decorated(ctx, subject).data()
      , name_of_decorated(ctx, itm).data()
      , "{part}");

    return true;
}

bool impl_can_unequip_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , const_entity_descriptor const itm_source
  , const_item_descriptor   const itm
  , const_entity_descriptor const itm_dest
  , body_part const*              part
  , string_buffer_base&           result
) noexcept {
    if (!check_definitions(result, subject, itm_source, itm, itm_dest)) {
        return false;
    }

    if (subject != itm_source) {
        BK_ASSERT(false && "TODO");
    }

    if (subject != itm_dest) {
        BK_ASSERT(false && "TODO");
    }

    auto const item_id = get_instance(itm);

    if (!part) {
        auto const last = itm_source->body_end();
        auto const it = std::find_if(itm_source->body_begin(), last
          , [&](body_part const& p) { return p.equip == item_id; });

        BK_ASSERT(it != last);

        part = std::addressof(*it);
    } else {
        BK_ASSERT(part->equip == get_instance(itm));
    }

    return impl_can_add_item(ctx, subject, itm, subject, result);
}

bool impl_try_unequip_item(
    const_context       const ctx
  , entity_descriptor   const subject
  , entity_descriptor   const itm_source
  , item_descriptor     const itm
  , entity_descriptor   const itm_dest
  , body_part*                part
  , string_buffer_base&       result
) noexcept {
    if (!impl_can_unequip_item(ctx, subject, itm_source, itm, itm_dest, part, result)) {
        return false;
    }

    itm_source->unequip(get_instance(itm));

    result.append("%s remove the %s from its %s."
      , name_of_decorated(ctx, subject).data()
      , name_of_decorated(ctx, itm).data()
      , "{part}");

    return true;
}

} // namespace detail

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
            *(std::end(key) - 3) = static_cast<char>('0' + i);
        } else {
            BK_ASSERT(n <= 99);
            *(std::end(key) - 2) = static_cast<char>('0' + i / 10);
            *(std::end(key) - 3) = static_cast<char>('0' + i % 10);
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

namespace {

template <typename Container, typename Predicate>
void entity_equip_impl(Container&& parts, item_pile& source, item_instance_id const id, Predicate pred) {
    using std::begin;
    using std::end;

    auto const last = end(parts);
    auto const it = std::find_if(begin(parts), last, pred);

    BK_ASSERT((it != last)
           && (it->equip == item_instance_id {}));

    auto itm = source.remove_item(id);
    BK_ASSERT(!!itm);

    it->equip = itm.release();
}

} // namespace

void entity::equip(item_instance_id const id) {
    entity_equip_impl(body_parts_, items(), id
      , [&](body_part const& part) { return part.is_free(); });
}

void entity::equip(body_part_id const part_id, item_instance_id const id) {
    entity_equip_impl(body_parts_, items(), id
      , [&](body_part const& part) { return part.id == part_id; });
}


void entity::unequip(item_instance_id const id) {
    auto const last = std::end(body_parts_);
    auto const it = std::find_if(begin(body_parts_), last
      , [&](body_part const& part) { return part.equip == id; });

    BK_ASSERT(it != last
           && it->equip == id);

    it->equip = item_instance_id {};

    items().add_item({id, item_deleter_});
}

} //namespace boken
