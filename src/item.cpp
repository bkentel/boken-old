#include "item.hpp"
#include "item_pile.hpp"
#include "item_properties.hpp"
#include "forward_declarations.hpp"
#include "utility.hpp" //  static_string_buffer

#include "bkassert/assert.hpp"

#include <tuple>
#include <numeric>

namespace boken {

item_id get_id(item_definition const& def) noexcept {
    return def.id;
}

namespace detail {

string_view impl_can_add_item(
    context               ctx
  , const_item_descriptor itm
  , const_item_descriptor dest
) noexcept {
    constexpr auto p_capacity = property(item_property::capacity);

    if (!itm.def) {
        return "{missing definition for item}";
    }

    if (!dest) {
        return "{missing definition for destination item}";
    }

    auto const dest_capacity = get_property_value_or(dest, p_capacity, 0);
    if (dest_capacity <= 0) {
        return "the destination is not a container";
    }

    auto const itm_capacity = get_property_value_or(itm, p_capacity, 0);
    if (itm_capacity > 0) {
        return "the item is too big";
    }

    if (dest.obj.items().size() + 1 > dest_capacity) {
        return "the destination is full";
    }

    return {};
}

string_view impl_can_remove_item(
    context               const ctx
  , const_item_descriptor const itm
  , const_item_descriptor const dest
) noexcept {
    return {};
}

} // namespace detail


void merge_into_pile(
    context         ctx
  , unique_item     itm_ptr
  , item_descriptor itm
  , item_descriptor pile
) {
    merge_into_pile(ctx, std::move(itm_ptr), itm, pile.obj.items());
}

//=====--------------------------------------------------------------------=====
//                               free functions
//=====--------------------------------------------------------------------=====
std::string name_of_decorated(
    context               const ctx
  , const_item_descriptor const itm
) {
    if (!itm) {
        return "{missing definition}";
    }

    static_string_buffer<128> buffer;
    buffer.append("%s", itm.def->name.data());

    auto const id_status = is_identified(itm);
    auto const capacity  = is_container(itm);

    if (capacity > 0) {
        if (id_status < 1) {
            buffer.append(" [?]");
        } else {
            auto const n = static_cast<int>(itm.obj.items().size());
            if (n == 0) {
                buffer.append(" <cr>[empty]</c>");
            } else {
                buffer.append(" [%d]", n);
            }
        }
    }

    return buffer.to_string();
}

uint32_t is_identified(const_item_descriptor const itm) noexcept {
    return get_property_value_or(itm, property(item_property::identified), 0);
}

uint32_t is_container(const_item_descriptor const itm) noexcept {
    return get_property_value_or(itm, property(item_property::capacity), 0);
}

std::string name_of_decorated(
    world           const& w
  , game_database   const& db
  , item            const& itm
  , item_definition const& def
) {
    static_string_buffer<128> buffer;

    auto const& base_name = def.name;
    buffer.append("%s", base_name.data());

    auto const id_status = is_identified(itm, def);
    auto const capacity  = is_container(itm, def);

    if (capacity > 0) {
        if (id_status < 1) {
            buffer.append(" [?]");
        } else {
            auto const n = static_cast<int>(itm.items().size());
            if (n == 0) {
                buffer.append(" <cr>[empty]</c>");
            } else {
                buffer.append(" [%d]", n);
            }
        }
    }

    return buffer.to_string();
}

string_view name_of(const_context const ctx, const_item_descriptor const i) noexcept {
    return i
      ? i.def->name
      : "{missing definition}";
}

std::string name_of_decorated(
    world         const& w
  , game_database const& db
  , item          const& itm
) {
    auto const def = find(db, itm.definition());
    if (!def) {
        BK_ASSERT(false); // TODO
    }

    return name_of_decorated(w, db, itm, *def);
}

std::string name_of_decorated(
    world            const& w
  , game_database    const& db
  , item_instance_id const  id
) {
    return name_of_decorated(w, db, find(w, id));
}

uint32_t is_container(item const& itm, item_definition const& def) noexcept {
    return get_property_value_or(itm, def, property(item_property::capacity), 0);
}

uint32_t is_container(game_database const& db, item const& itm) noexcept {
    return get_property_value_or(db, itm, property(item_property::capacity), 0);
}

uint32_t is_identified(game_database const& db, item const& itm) noexcept {
    return get_property_value_or(db, itm, property(item_property::identified), 0);
}

uint32_t is_identified(item const& itm, item_definition const& def) noexcept {
    return get_property_value_or(itm, def, property(item_property::identified), 0);
}

std::string item_description(
    world           const& w
  , game_database   const& db
  , item            const& itm
  , item_definition const& def
) {
    static_string_buffer<256> buffer;

    buffer.append("<cr>%s</c>", name_of(const_context {w, db}, {db, itm}).data());

    auto const we = weight_of_exclusive(db, itm);

    auto const id_status = is_identified(itm, def);
    auto const capacity  = is_container(itm, def);

    if (capacity > 0) {
        auto const wi = weight_of_inclusive(w, db, itm);
        buffer.append("\nWeight: %d (%d)", wi, we);

        auto const n = static_cast<int>(itm.items().size());

        if (id_status > 0) {
            buffer.append("\nContains %d of %d items", n , capacity);
        } else {
            buffer.append("\nContains ? items");
        }
    } else {
        buffer.append("\nWeight: %d", we);
    }

    return buffer.to_string();
}

std::string item_description(
    world            const& w
  , game_database    const& db
  , item_instance_id const  id
) {
    auto const& itm = find(w, id);
    auto const& def = find(db, itm.definition());

    BK_ASSERT(!!def); // TODO

    return item_description(w, db, itm, *def);
}

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

    return static_cast<int32_t>(weight * stack);
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

item_property_value get_property_value_or(
    const_item_descriptor const itm
  , item_property_id      const property
  , item_property_value   const fallback
) noexcept {
    return itm
      ? itm.obj.property_value_or(*itm.def, property, fallback)
      : fallback;
}

item_property_value get_property_value_or(
    item const&               itm
  , item_definition const&    def
  , item_property_id const    property
  , item_property_value const fallback
) noexcept {
    return itm.property_value_or(def, property, fallback);
}

item_property_value get_property_value_or(
    game_database const&      db
  , item const&               itm
  , item_property_id const    property
  , item_property_value const fallback
) noexcept {
    return itm.property_value_or(db, property, fallback);
}

item_property_value get_property_value_or(
    game_database const&      db
  , item_id                   id
  , item_property_id const    property
  , item_property_value const fallback
) noexcept {
    auto const def = find(db, id);
    return def
      ? get_property_value_or(property, fallback, def->properties)
      : fallback;
}

item_property_value get_property_value_or(
    world const&              w
  , game_database const&      db
  , item_instance_id const    id
  , item_property_id const    property
  , item_property_value const fallback
) noexcept {
    return get_property_value_or(db, find(w, id), property, fallback);
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
    context         const ctx
  , unique_item           itm_ptr
  , item_descriptor const itm
  , item_pile&            pile
) {
    BK_ASSERT(!!itm_ptr);

    // the default action on any failure is to preserve the item and add it to
    // the pile
    auto on_exit = BK_SCOPE_EXIT {
        pile.add_item(std::move(itm_ptr));
    };

    // if the item doesn't have a valid id, preserve the item anyway and add it
    // to the pile.
    if (!itm) {
        return;
    }

    constexpr auto p_max_stack = property(item_property::stack_size);
    constexpr auto p_cur_stack = property(item_property::current_stack_size);

    // if the item can't be stacked, just add the item to the pile
    if (!get_property_value_or(itm, p_max_stack, 0)) {
        return;
    }

    auto src_cur_stack = get_property_value_or(itm, p_cur_stack, 0);
    BK_ASSERT(src_cur_stack > 0); // no zero sized stacks

    for (auto const& id : pile) {
        auto const i = item_descriptor {ctx, id};

        // different item
        if (i.def != itm.def) {
            continue;
        }

        auto const max_stack = get_property_value_or(i, p_max_stack, 0);
        auto const cur_stack = get_property_value_or(i, p_cur_stack, 0);

        // no space in the stack to merge quantity
        if (cur_stack >= max_stack) {
            BK_ASSERT(cur_stack <= max_stack);
            continue;
        }

        auto const spare_stack = max_stack - cur_stack;
        auto const n = std::min(src_cur_stack, spare_stack);

        src_cur_stack -= n;
        i.obj.add_or_update_property({p_cur_stack, cur_stack + n});

        if (src_cur_stack <= 0) {
            on_exit.dismiss();
            return;
        }
    }

    BK_ASSERT(src_cur_stack > 0);
    itm.obj.add_or_update_property({p_cur_stack, src_cur_stack});
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
