#pragma once

#include "types.hpp"
#include "math_types.hpp"
#include "context_fwd.hpp"

#include "bkassert/assert.hpp"

#include <utility>
#include <type_traits>
#include <cstddef>

namespace boken { class level; }

namespace boken {

//===------------------------------------------------------------------------===
//! Convenience wrapper around the world state and game database (both of which
//! are used extensively throughout the codebase).
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
      : w  {w_}
      , db {db_}
    {
    }

    world_t&             w;
    game_database const& db;
};

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

//===------------------------------------------------------------------------===
//
//===------------------------------------------------------------------------===
template <bool Const>
struct level_location_base {
    using level_t = std::conditional_t<Const, level const, level>;

    constexpr level_location_base(level_t& lvl_, point2i32 const where) noexcept
      : lvl {lvl_}
      , p   {where}
    {
    }

    template <bool C, typename = std::enable_if_t<!C || Const>>
    constexpr level_location_base(level_location_base<C> const other) noexcept
      : lvl {other.lvl}
      , p   {other.p}
    {
    }

    auto* operator->() const noexcept { return std::addressof(lvl); }
    auto* operator->()       noexcept { return std::addressof(lvl); }

    constexpr operator point2i32() const noexcept { return p; }

    level_t&  lvl;
    point2i32 p;
};

//=====--------------------------------------------------------------------=====
// check_definitions
//=====--------------------------------------------------------------------=====
namespace detail {

template <typename T, typename Out>
bool check_definition(T const& obj, Out&& out, char const* const msg) noexcept {
    return !!obj || ((void)out.append("%s", msg), false);
}

template <typename Out>
bool check_definition(const_entity_descriptor const& e, Out&& out) noexcept {
    return check_definition(e, out, "{missing definition for entity}");
}

template <typename Out>
bool check_definition(const_item_descriptor const& i, Out&& out) noexcept {
    return check_definition(i, out, "{missing definition for item}");
}

} // namespace detail

//! Return whether all of the arguments are valid definitions. For any invalid
//! arguments, out.append(...) is called.
template <typename... Args, typename Out>
bool check_definitions(Out&& out, Args const&... args) noexcept {
    static_assert(sizeof...(args) > 0, "");
    using expand_t = bool const [];

    bool ok = true;
    (void)expand_t {(ok = detail::check_definition(args, out))...};
    return ok;
}

//=====--------------------------------------------------------------------=====
// Function parameter types: subject, object, etc
//=====--------------------------------------------------------------------=====
namespace detail {

template <typename T, param_class ClassT>
struct param_t {
    constexpr explicit param_t(T v)
      : value {std::move(v)}
    {
    }

    template <typename U
            , typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr param_t(param_t<U, ClassT> v)
      : value {std::move(v.value)}
    {
    }

    template <typename U,
              typename = std::enable_if_t<std::is_convertible<T, U>::value>>
    constexpr operator U() const noexcept { return value; }

    T value;
};

} // namespace detail;

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
