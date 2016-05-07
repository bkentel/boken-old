#pragma once

#include "config.hpp"
#include "types.hpp"
#include "bkassert/assert.hpp"

#include <type_traits>

namespace boken { class world; }
namespace boken { class level; }
namespace boken { class game_database; }
namespace boken { class entity; }
namespace boken { class item; }
namespace boken { struct entity_definition; }
namespace boken { struct item_definition; }

namespace boken {

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
entity_id get_id(entity            const& e  ) noexcept;
item_id   get_id(item              const& i  ) noexcept;
entity_id get_id(entity_definition const& def) noexcept;
item_id   get_id(item_definition   const& def) noexcept;

item   const& find(world const& w, item_instance_id   id) noexcept;
entity const& find(world const& w, entity_instance_id id) noexcept;
item&         find(world&       w, item_instance_id   id) noexcept;
entity&       find(world&       w, entity_instance_id id) noexcept;

item_definition   const* find(game_database const& db, item_id   id) noexcept;
entity_definition const* find(game_database const& db, entity_id id) noexcept;

//===------------------------------------------------------------------------===
//
//===------------------------------------------------------------------------===
template <bool Const>
struct context_base {
    using world_t = std::conditional_t<Const, world const, world>;

    template <bool C, typename = std::enable_if_t<!C || Const>>
    context_base(context_base<C> const other) noexcept
      : w  {other.w}
      , db {other.db}
    {
    }

    context_base(world_t& w_, game_database const& db_) noexcept
      : w {w_}
      , db {db_}
    {
    }

    world_t&             w;
    game_database const& db;
};

using context       = context_base<false>;
using const_context = context_base<true>;

//===------------------------------------------------------------------------===
//
//===------------------------------------------------------------------------===
template <typename Object, typename Definition>
struct descriptor_base {
    static constexpr bool is_const = std::is_const<Object>::value;

    // construct const from non-const
    template <typename T, typename = std::enable_if_t<
         std::is_same<std::remove_cv_t<T>, std::remove_cv_t<Object>>::value
     && (!descriptor_base<T, Definition>::is_const || is_const)
    >>
    descriptor_base(descriptor_base<T, Definition> other) noexcept
      : obj {other.obj}
      , def {other.def}
    {
    }

    template <typename T, typename Tag>
    descriptor_base(
        std::conditional_t<is_const, world const, world>& w
      , game_database const&       db
      , tagged_value<T, Tag> const id
    ) noexcept
      : obj {find(w, id)}
      , def {find(db, get_id(obj))}
    {
    }

    template <typename T, typename Tag>
    descriptor_base(context_base<is_const> const ctx, tagged_value<T, Tag> const id) noexcept
      : descriptor_base {ctx.w, ctx.db, id}
    {
    }

    descriptor_base(game_database const& db, Object& object) noexcept
      : obj {object}
      , def {find(db, get_id(obj))}
    {
    }

    descriptor_base(context_base<is_const> const ctx, Object& object) noexcept
      : descriptor_base {ctx.db, object}
    {
    }

    descriptor_base(Object& object, Definition const& definition) noexcept
      : obj {object}
      , def {&definition}
    {
        BK_ASSERT(get_id(definition) == get_id(object));
    }

    operator bool() const noexcept {
        return !!def;
    }

    Object&           obj;
    Definition const* def;
};

using item_descriptor         = descriptor_base<item,         item_definition>;
using const_item_descriptor   = descriptor_base<item const,   item_definition>;
using entity_descriptor       = descriptor_base<entity,       entity_definition>;
using const_entity_descriptor = descriptor_base<entity const, entity_definition>;

//===------------------------------------------------------------------------===
//
//===------------------------------------------------------------------------===
template <bool Const>
struct level_location_base {
    using type = std::conditional_t<Const, std::add_const_t<level>, level>;

    level_location_base(type& level, point2i32 const where)
      : lvl {level}
      , p   {where}
    {
    }

    template <bool C, typename = std::enable_if_t<Const || !C>>
    level_location_base(level_location_base<C> other)
      : lvl {other.lvl}
      , p   {other.p}
    {
    }

    constexpr explicit operator bool() const noexcept { return true; }

    std::conditional_t<Const, std::add_const_t<level>, level>& lvl;
    point2i32 p;
};

using level_location = level_location_base<false>;
using const_level_location = level_location_base<true>;

template <typename UnaryF>
bool not_empty_or(UnaryF const f, string_view const s) noexcept {
    if (!s.empty()) {
        f(s);
        return false;
    }

    return true;
}

} // namespace boken
