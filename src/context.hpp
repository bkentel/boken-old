#pragma once

#include "config.hpp"
#include "types.hpp"
#include "math_types.hpp"
#include "context_fwd.hpp"

#include "bkassert/assert.hpp"

#include <utility>
#include <type_traits>
#include <cstddef>

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
entity_id get_id(const_entity_descriptor e) noexcept;
item_id   get_id(const_item_descriptor i) noexcept;

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

    template <typename T>
    bool operator==(descriptor_base<T, Definition> const& other) const noexcept {
        return (std::addressof(other.obj) == std::addressof(obj))
            && (other.def == def);
    }

    template <typename T>
    bool operator!=(descriptor_base<T, Definition> const& other) const noexcept {
        return !(*this == other);
    }

    Object* operator->() const noexcept {
        return &obj;
    }

    Object* operator->() noexcept {
        return &obj;
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

    constexpr level_location_base(type& level, point2i32 const where) noexcept
      : lvl {level}
      , p   {where}
    {
    }

    template <bool C, typename = std::enable_if_t<Const || !C>>
    constexpr level_location_base(level_location_base<C> other) noexcept
      : lvl {other.lvl}
      , p   {other.p}
    {
    }

    auto* operator->() const noexcept { return std::addressof(lvl); }
    auto* operator->()       noexcept { return std::addressof(lvl); }

    constexpr explicit operator bool() const noexcept { return true; }
    constexpr operator point2i32() const noexcept { return p; }

    std::conditional_t<Const, std::add_const_t<level>, level>& lvl;
    point2i32 p;
};

using level_location = level_location_base<false>;
using const_level_location = level_location_base<true>;

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====

namespace detail {

template <typename T, typename Out>
bool check_definition(T&& obj, Out&& out, char const* const msg) noexcept {
    if (!obj) {
        out.append("%s", msg);
        return false;
    }

    return true;
}

template <typename Out>
bool check_definition(const_entity_descriptor const e, Out&& out) noexcept {
    return check_definition(e, out, "{missing definition for entity}");
}

template <typename Out>
bool check_definition(const_item_descriptor const i, Out&& out) noexcept {
    return check_definition(i, out, "{missing definition for item}");
}

} // namespace detail

template <typename... Args, typename Out>
bool check_definitions(Out&& out, Args&&... args) noexcept {
    static_assert(sizeof...(args) > 0, "");
    bool result = true;
    int const arr[] {(result &= detail::check_definition(
        std::forward<Args>(args), std::forward<Out>(out)), 0)...};
    (void)arr;
    return result;
}

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
namespace detail {

enum class param_class {
    subject
  , object
  , at
  , from
  , to
};

//=====--------------------------------------------------------------------=====
template <typename T, param_class ClassT>
struct param_t {
    explicit param_t(T v)
      : value {v}
    {
    }

    template <typename U
            , typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    param_t(param_t<U, ClassT> v)
      : value {v.value}
    {
    }

    template <typename U,
              typename = std::enable_if_t<std::is_convertible<T, U>::value>>
    operator U() const noexcept { return value; }

    T value;
};

} // namespace detail;


//=====--------------------------------------------------------------------=====
template <typename T>
using subject_t = detail::param_t<T, detail::param_class::subject>;

template <typename T>
using object_t = detail::param_t<T, detail::param_class::object>;

template <typename T>
using at_t = detail::param_t<T, detail::param_class::at>;

template <typename T>
using from_t = detail::param_t<T, detail::param_class::from>;

template <typename T>
using to_t = detail::param_t<T, detail::param_class::to>;

//=====--------------------------------------------------------------------=====
template <typename T>
constexpr auto p_subject(T const value) noexcept {
    return subject_t<T> {value};
}

template <typename T>
constexpr auto p_object(T const value) noexcept {
    return object_t<T> {value};
}

template <typename T>
constexpr auto p_from(T const value) noexcept {
    return from_t<T> {value};
}

template <typename T>
constexpr auto p_to(T const value) noexcept {
    return to_t<T> {value};
}

} // namespace boken
