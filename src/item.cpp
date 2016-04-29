#include "item.hpp"
#include "item_pile.hpp"
#include "item_properties.hpp"
#include "forward_declarations.hpp"

#include "bkassert/assert.hpp"

#include <tuple>
#include <numeric>

namespace boken {

//=====--------------------------------------------------------------------=====
//                               free functions
//=====--------------------------------------------------------------------=====
item_id get_pile_id(game_database const& db) noexcept {
    auto const pile_def = find(db, make_id<item_id>("pile"));
    return pile_def
      ? pile_def->id
      : item_id {};
}

item_id get_pile_id(
    world     const& w
  , item_pile const& pile
  , item_id   const  pile_id
) noexcept {
    BK_ASSERT(!pile.empty());

    return (pile.size() == 1u)
      ? find(w, *pile.begin()).definition()
      : pile_id;
}

int32_t weight_of_exclusive(
    game_database const& db
  , item          const& itm
) noexcept {
    constexpr auto prop_weight = property(item_property::weight);
    constexpr auto prop_stack  = property(item_property::current_stack_size);

    auto const weight = get_property_value_or(db, itm, prop_weight, 0);
    auto const stack  = get_property_value_or(db, itm, prop_stack,  1);

    return weight * stack;
}

int32_t weight_of_exclusive(
    world            const& w
  , game_database    const& db
  , item_instance_id const  id
) noexcept {
    return weight_of_exclusive(db, find(w, id));
}

int32_t weight_of_inclusive(
    world         const& w
  , game_database const& db
  , item          const& itm
) noexcept {
    auto const first = begin(itm.items());
    auto const last  = end(itm.items());

    auto const w0 = weight_of_exclusive(db, itm);
    auto const w1 = std::accumulate(first, last, int32_t {0}
        , [&](int32_t const sum, item_instance_id const id) {
            return sum + weight_of_inclusive(w, db, id);
        });

    return w0 + w1;
}

int32_t weight_of_inclusive(
    world            const& w
  , game_database    const& db
  , item_instance_id const  id
) noexcept {
    return weight_of_inclusive(w, db, find(w, id));
}

string_view name_of(game_database const& db, item const& i) noexcept {
    return name_of(db, i.definition());
}

bool can_add_item(
    game_database const& db
  , item          const& dest
  , item          const& itm
) noexcept {
    auto const dest_capacity =
        get_property_value_or(db, dest, property(item_property::capacity), 0);

    // the destination is not a container
    if (dest_capacity <= 0) {
        return false;
    }

    // the destination is full
    if (dest.items().size() + 1 > dest_capacity) {
        return false;
    }

    auto const itm_capacity =
        get_property_value_or(db, itm, property(item_property::capacity), 0);

    // the item to add is itself a container
    if (itm_capacity > 0) {
        return false;
    }

    return true;
}

bool can_add_item(
    game_database   const& db
  , item            const& dest
  , item_definition const& def
) noexcept {
    return can_add_item(
        db, dest, item {item_instance_id {}, def.id});
}

item_property_value get_property_value_or(
    game_database const&      db
  , item const&               i
  , item_property_id const    property
  , item_property_value const fallback
) noexcept {
    return i.property_value_or(db, property, fallback);
}

item_property_value get_property_value_or(
    game_database const&      db
  , item_id const             id
  , item_property_id const    property
  , item_property_value const fallback
) noexcept {
    auto* const def = find(db, id);
    return def
      ? get_property_value_or(*def, property, fallback)
      : fallback;
}

item_property_value get_property_value_or(
    item_definition const& def
  , item_property_id       property
  , item_property_value    fallback
) noexcept {
    return def.properties.value_or(property, fallback);
}

item_instance_id get_instance(item const& i) noexcept {
    return i.instance();
}

item_id get_id(item const& i) noexcept {
    return i.definition();
}

item_pile& get_items(item& i) noexcept {
    return i.items();
}

item_pile const& get_items(item const& i) noexcept {
    return i.items();
}

namespace {

item create_object(
    item_instance_id const  instance
  , item_definition  const& def
  , random_state&           rng
) {
    item result {instance, def.id};

    //
    // check if the item type can be stacked, and if so set its current stack
    // size.
    //
    auto const stack_size = def.properties.value_or(
        property(item_property::stack_size), 0);

    if (stack_size > 0) {
        result.add_or_update_property(
            property(item_property::current_stack_size), 1);
    }

    return result;
}

} // namespace

unique_item create_object(
    world&                 w
  , item_definition const& def
  , random_state&          rng
) {
    return create_object(w, [&](item_instance_id const instance) {
        return create_object(instance, def, rng);
    });
}

void merge_into_pile(
    world& w
  , game_database const& db
  , unique_item&& itm
  , item_pile& pile
) {
    BK_ASSERT(!!itm);
    auto const a_instance = itm.get();

    // {item&, item_id, item_definition*}
    auto const get_info = [&](item_instance_id const instance_id) {
        auto&       itm = find(w, instance_id);
        auto  const id  = itm.definition();
        auto* const def = find(db, id);

        return std::make_tuple(std::ref(itm), id, def);
    };

    auto const get_current_stack = [&](item const& i) {
        return i.property_value_or(db, property(item_property::current_stack_size), 0);
    };

    auto const set_current_stack = [&](item& i, item_property_value const n) {
        auto const result =
            i.add_or_update_property(property(item_property::current_stack_size), n);
        BK_ASSERT(!result);
    };

    auto const a_info    = get_info(a_instance);
    auto const a_def_ptr = std::get<2>(a_info);

    // if no definition can be found, just add the item to the pile
    if (!a_def_ptr) {
        pile.add_item(std::move(itm));
        return;
    }

    // if the item can't be stacked, just add the item to the pile
    auto const max_stack = a_def_ptr->properties.value_or(property(item_property::stack_size), 0);
    if (max_stack <= 0) {
        pile.add_item(std::move(itm));
        return;
    }

    auto const a_def       = std::get<1>(a_info);
    auto&      a_itm       = std::get<0>(a_info);
    auto       a_cur_stack = item_property_value {0};

    // find any compatible items and merge as much quantity into them as
    // possible
    for (item_instance_id const b_instance : pile) {
        auto const b_info = get_info(b_instance);
        auto const b_def  = std::get<1>(b_info);

        // reject if definitions don't match
        if (a_def != b_def) {
            continue;
        }

        auto&      b_itm       = std::get<0>(b_info);
        auto const b_cur_stack = get_current_stack(b_itm);

        BK_ASSERT(max_stack >= b_cur_stack);
        auto const b_remaining = max_stack - b_cur_stack;

        // reject if no more room in this stack
        if (b_remaining <= 0) {
            continue;
        }

        // first time -- get the value
        if (a_cur_stack <= 0) {
            a_cur_stack = get_current_stack(a_itm);
        }

        BK_ASSERT(a_cur_stack > 0);

        // move as much quantity from a -> b as possible
        auto const n = std::min(b_remaining, a_cur_stack);
        set_current_stack(b_itm, b_cur_stack + n);
        a_cur_stack -= n;

        // if there is nothing left in a, destroy it; we're done
        if (a_cur_stack <= 0) {
            itm.reset();
            break;
        }
    }

    // quantity in a was modified, and not all of it could be merged into the
    // pile; update a's final quantity
    if (a_cur_stack > 0) {
        set_current_stack(a_itm, a_cur_stack);
    }

    // quantity in a either couldn't fully be merged, or was only partially
    // merged; add the item as a new item in the pile.
    if (itm) {
        pile.add_item(std::move(itm));
    }
}

//=====--------------------------------------------------------------------=====
//                                  item
//=====--------------------------------------------------------------------=====

//=====--------------------------------------------------------------------=====
//                                  item_pile
//=====--------------------------------------------------------------------=====

item_pile::~item_pile() {
    BK_ASSERT(items_.empty() || !!deleter_);

    for (auto const& id : items_) {
        unique_item {id, *deleter_};
    }
}

item_pile::item_pile()
{
}

item_instance_id item_pile::operator[](size_t const index) const noexcept {
    BK_ASSERT(index < items_.size());
    return items_[index];
}

void item_pile::add_item(unique_item item) {
    if (!deleter_) {
        deleter_ = &item.get_deleter();
    }

    items_.push_back(item.release());
}

unique_item item_pile::remove_item(item_instance_id const id) {
    BK_ASSERT(!!deleter_);

    auto const it = std::find(std::begin(items_), std::end(items_), id);
    if (it == std::end(items_)) {
        return unique_item {item_instance_id {} , *deleter_};
    }

    items_.erase(it);
    return unique_item {id, *deleter_};
}

unique_item item_pile::remove_item(size_t const pos) {
    BK_ASSERT(pos < items_.size());
    BK_ASSERT(!!deleter_);

    auto const id = items_[pos];
    items_.erase(std::begin(items_) + static_cast<ptrdiff_t>(pos));
    return unique_item {id, *deleter_};
}

} //namespace boken
