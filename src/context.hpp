#pragma once

#include "types.hpp"
#include "bkassert/assert.hpp"

namespace boken { class world; }
namespace boken { class game_database; }
namespace boken { struct entity_definition; }
namespace boken { struct item_definition; }
namespace boken { class entity; }
namespace boken { class item; }

namespace boken {

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

using item_descriptor       = descriptor_base<item,       item_definition>;
using const_item_descriptor = descriptor_base<item const, item_definition>;

using entity_descriptor       = descriptor_base<entity,       entity_definition>;
using const_entity_descriptor = descriptor_base<entity const, entity_definition>;

} // namespace boken
