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

struct context {
    world&               w;
    game_database const& db;
};

template <typename Object, typename Definition>
struct descriptor_base {
    // construct const from non-const
    template <typename T, typename = std::enable_if_t<
         std::is_same<std::remove_cv_t<T>, std::remove_cv_t<Object>>::value
     && (std::is_const<Object>::value || !std::is_const<T>::value)
    >>
    descriptor_base(descriptor_base<T, Definition> other) noexcept
      : obj {other.obj}
      , def {other.def}
    {
    }

    template <typename T, typename Tag>
    descriptor_base(
        world&                     w
      , game_database const&       db
      , tagged_value<T, Tag> const id
    ) noexcept
      : obj {find(w, id)}
      , def {find(db, get_id(obj))}
    {
    }

    template <typename T, typename Tag>
    descriptor_base(context const ctx, tagged_value<T, Tag> const id) noexcept
      : descriptor_base {ctx.w, ctx.db, id}
    {
    }

    descriptor_base(game_database const& db, Object& object) noexcept
      : obj {object}
      , def {find(db, get_id(obj))}
    {
    }

    descriptor_base(context const ctx, Object& object) noexcept
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
