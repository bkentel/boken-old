#pragma once

#include "types.hpp"
#include "utility.hpp"

#include "math_types.hpp"

#include <type_traits>
#include <functional>
#include <cmath>
#include <cstdint>

namespace boken {

//------------------------------------------------------------------------------
//! Return the value of @p n casted to the specified type To (or its underlying
//! type if no type is given for To).
template <
    typename To = void
  , typename From
  , typename TagAxis
  , typename TagType
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>>
constexpr Result value_cast(basic_1_tuple<From, TagAxis, TagType> const n) noexcept {
    static_assert(is_safe_integer_conversion<From, Result>::value, "");
    return static_cast<Result>(n.value_);
}

template <
    typename To = void
  , typename From
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>
  , typename = std::enable_if_t<std::is_arithmetic<From>::value>>
constexpr Result value_cast(From const n) noexcept {
    static_assert(is_safe_integer_conversion<From, Result>::value, "");
    return static_cast<Result>(n);
}

//------------------------------------------------------------------------------
//! Return the value of @p n casted to the specified type To (or its underlying
//! type if no type is given for To).
//! @see value_cast
//! @note This function allows unsafe conversions.
template <
    typename To = void
  , typename From
  , typename TagAxis
  , typename TagType
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>
  , typename = std::enable_if_t<!std::is_arithmetic<From>::value>>
constexpr Result value_cast_unsafe(basic_1_tuple<From, TagAxis, TagType> const n) noexcept {
    return static_cast<Result>(n.value_);
}

template <
    typename To = void
  , typename From
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>
  , typename = std::enable_if_t<std::is_arithmetic<From>::value>>
constexpr Result value_cast_unsafe(From const n) noexcept {
    return static_cast<Result>(n);
}

//=====--------------------------------------------------------------------=====
//                           Arithmetic Operations
//=====--------------------------------------------------------------------=====

template <typename T, typename U>
struct has_common_safe_integer : std::integral_constant<bool
  , std::is_integral<T>::value && std::is_integral<U>::value
 && (is_safe_integer_conversion<T, U>::value
  || is_safe_integer_conversion<U, T>::value)>
{
};

template <typename T, typename U, bool Check = false>
struct common_safe_integer {
    static_assert(!Check || has_common_safe_integer<T, U>::value, "");

    using type = std::conditional_t<
        has_common_safe_integer<T, U>::value
      , std::common_type_t<T, U>
      , void>;
};

template <typename T, typename U, bool Check = false>
using common_safe_integer_t = typename common_safe_integer<T, U, Check>::type;

//------------------------------------------------------------------------------
namespace detail {

template <
    typename ResultAxis
  , typename ResultType
  , typename T, typename U
  , typename Compute>
inline constexpr auto compute_scalar(T const x, U const y, Compute f) noexcept {
    using common_t = common_safe_integer_t<T, U, true>;
    using result_t = basic_1_tuple<common_t, ResultAxis, ResultType>;

    return result_t {static_cast<common_t>(
        f(value_cast(x), value_cast(y)))};
}

template <
    typename ResultAxis
  , typename ResultType
  , typename T, typename U
  , typename Compute>
inline constexpr auto compute(T const x, U const y, Compute f) noexcept {
    return compute_scalar<ResultAxis, ResultType>(value_cast(x), value_cast(y), f);
}

} //namespace detail

template <typename T, typename U, typename TagAxis, typename TagType>
constexpr auto operator*(
    basic_1_tuple<T, TagAxis, TagType> const x, U const c
) noexcept {
    return detail::compute<TagAxis, TagType>(x, c, std::multiplies<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType>
constexpr auto operator/(
    basic_1_tuple<T, TagAxis, TagType> const x, U const c
) noexcept {
    return detail::compute<TagAxis, TagType>(x, c, std::divides<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr auto operator+(
    basic_1_tuple<T, TagAxis, tag_vector> const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, tag_vector>(x, y, std::plus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr auto operator-(
    basic_1_tuple<T, TagAxis, tag_vector> const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, tag_vector>(x, y, std::minus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr auto operator+(
    basic_1_tuple<T, TagAxis, tag_point>  const p
  , basic_1_tuple<U, TagAxis, tag_vector> const v
) noexcept {
    return detail::compute<TagAxis, tag_point>(p, v, std::plus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr auto operator-(
    basic_1_tuple<T, TagAxis, tag_point>  const p
  , basic_1_tuple<U, TagAxis, tag_vector> const v
) noexcept {
    return detail::compute<TagAxis, tag_point>(p, v, std::minus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr auto operator-(
    basic_1_tuple<T, TagAxis, tag_point> const p
  , basic_1_tuple<U, TagAxis, tag_point> const q
) noexcept {
    return detail::compute<TagAxis, tag_vector>(p, q, std::minus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_vector>& operator+=(
    basic_1_tuple<T, TagAxis, tag_vector>&      x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x + y);
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_vector>& operator-=(
    basic_1_tuple<T, TagAxis, tag_vector>&      x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x - y);
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_point>& operator+=(
    basic_1_tuple<T, TagAxis, tag_point>&       x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x + y);
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_point>& operator-=(
    basic_1_tuple<T, TagAxis, tag_point>&       x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x - y);
}

template <typename T, typename U, typename TagAxis, typename TagType>
constexpr basic_1_tuple<T, TagAxis, TagType>& operator*=(
    basic_1_tuple<T, TagAxis, TagType>& x, U const c
) noexcept {
    return (x = x * c);
}

template <typename T, typename U, typename TagAxis, typename TagType>
constexpr basic_1_tuple<T, TagAxis, TagType>& operator/=(
    basic_1_tuple<T, TagAxis, TagType>& x, U const c
) noexcept {
    return (x = x / c);
}

//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator+(vec2<T> const u, vec2<U> const v) noexcept {
    return vec2<common_safe_integer_t<T, U, true>> {u.x + v.x, u.y + v.y};
}

template <typename T, typename U>
constexpr auto operator-(vec2<T> const u, vec2<U> const v) noexcept {
    return vec2<common_safe_integer_t<T, U, true>> {u.x - v.x, u.y - v.y};
}

template <typename T, typename U>
constexpr auto operator+(point2<T> const p, vec2<U> const v) noexcept {
    return point2<common_safe_integer_t<T, U, true>> {p.x + v.x, p.y + v.y};
}

template <typename T, typename U>
constexpr auto operator-(point2<T> const p, vec2<U> const v) noexcept {
    return point2<common_safe_integer_t<T, U, true>> {p.x + v.x, p.y + v.y};
}

template <typename T, typename U>
constexpr auto operator-(point2<T> const p, point2<U> const q) noexcept {
    return vec2<common_safe_integer_t<T, U, true>> {p.x - q.x, p.y - q.y};
}

template <typename T, typename U>
constexpr auto operator+(axis_aligned_rect<T> const r, vec2<U> const v) noexcept {
    return axis_aligned_rect<common_safe_integer_t<T, U, true>> {
        r.x0 + v.x, r.y0 + v.y
      , r.x1 + v.x, r.y1 + v.y
    };
}

template <typename T, typename U>
constexpr auto operator-(axis_aligned_rect<T> const r, vec2<U> const v) noexcept {
    return axis_aligned_rect<common_safe_integer_t<T, U, true>> {
        r.x0 - v.x, r.y0 - v.y
      , r.x1 - v.x, r.y1 - v.y
    };
}

template <typename T, typename U, typename TagType>
constexpr auto operator*(
    basic_2_tuple<T, TagType> const p, U const c
) noexcept {
    return basic_2_tuple<common_safe_integer_t<T, U, true>, TagType> {
        detail::compute<tag_axis_x, TagType>(p.x, c, std::multiplies<> {})
      , detail::compute<tag_axis_y, TagType>(p.y, c, std::multiplies<> {})};
}

template <typename T, typename U, typename TagType>
constexpr auto operator/(
    basic_2_tuple<T, TagType> const p, U const c
) noexcept {
    return basic_2_tuple<common_safe_integer_t<T, U, true>, TagType> {
        detail::compute<tag_axis_x, TagType>(p.x, c, std::divides<> {})
      , detail::compute<tag_axis_y, TagType>(p.y, c, std::divides<> {})};
}

template <typename T, typename U>
constexpr vec2<T>& operator+=(vec2<T>& u, vec2<U> const v) noexcept {
    return (u = u + v);
}

template <typename T, typename U>
constexpr vec2<T>& operator-=(vec2<T>& u, vec2<U> const v) noexcept {
    return (u = u - v);
}

template <typename T, typename U>
constexpr point2<T>& operator+=(point2<T>& p, vec2<U> const v) noexcept {
    return (p = p + v);
}

template <typename T, typename U>
constexpr point2<T>& operator-=(point2<T>& p, vec2<U> const v) noexcept {
    return (p = p - v);
}

template <typename T, typename U>
constexpr axis_aligned_rect<T> operator+=(axis_aligned_rect<T>& r, vec2<U> const v) noexcept {
    return (r = r + v);
}

template <typename T, typename U>
constexpr axis_aligned_rect<T> operator-=(axis_aligned_rect<T>& r, vec2<U> const v) noexcept {
    return (r = r +-v);
}

template <typename T, typename U, typename TagType>
constexpr basic_2_tuple<T, TagType>& operator*=(
    basic_2_tuple<T, TagType>& p, U const c
) noexcept {
    return (p = p * c);
}

template <typename T, typename U, typename TagType>
constexpr basic_2_tuple<T, TagType>& operator/=(
    basic_2_tuple<T, TagType>& p, U const c
) noexcept {
    return (p = p / c);
}

//=====--------------------------------------------------------------------=====
//                           Comparison Operations
//=====--------------------------------------------------------------------=====

namespace detail {

template <typename T, typename U, typename TagAxis, typename TagType, typename Compare>
inline constexpr bool compare(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
  , Compare f
) noexcept {
    using common_t = common_safe_integer_t<T, U, true>;
    return f(value_cast<common_t>(x), value_cast<common_t>(y));
}

} //namespace detail

//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis, typename TagType>
inline constexpr bool operator==(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
) noexcept {
    return detail::compare(x, y, std::equal_to<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType>
inline constexpr bool operator<(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
) noexcept {
    return detail::compare(x, y, std::less<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType>
inline constexpr bool operator<=(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
) noexcept {
    return detail::compare(x, y, std::less_equal<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType>
inline constexpr bool operator>(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
) noexcept {
    return detail::compare(x, y, std::greater<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType>
inline constexpr bool operator>=(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
) noexcept {
    return detail::compare(x, y, std::greater_equal<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType>
inline constexpr bool operator!=(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
) noexcept {
    return detail::compare(x, y, std::not_equal_to<> {});
}

//------------------------------------------------------------------------------
template <typename T, typename U>
inline constexpr bool operator==(point2<T> const p, point2<U> const q) noexcept {
    return (p.x == q.x) && (p.y == q.y);
}

template <typename T, typename U>
inline constexpr bool operator!=(point2<T> const p, point2<U> const q) noexcept {
    return !(p == q);
}

//------------------------------------------------------------------------------
template <typename T, typename U>
inline constexpr bool operator==(vec2<T> const u, vec2<U> const v) noexcept {
    return (u.x == v.x) && (u.y == v.y);
}

template <typename T, typename U>
inline constexpr bool operator!=(vec2<T> const u, vec2<U> const v) noexcept {
    return !(u == v);
}

//------------------------------------------------------------------------------
template <typename T, typename U> inline constexpr
bool operator==(axis_aligned_rect<T> const lhs, axis_aligned_rect<U> const rhs) noexcept {
    return (lhs.x0 == rhs.x0)
        && (lhs.y0 == rhs.y0)
        && (lhs.x1 == rhs.x1)
        && (lhs.y1 == rhs.y1);
}

template <typename T, typename U> inline constexpr
bool operator!=(axis_aligned_rect<T> const lhs, axis_aligned_rect<U> const rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace boken

namespace boken {

template <typename T> inline constexpr
auto square_of(T const n) noexcept {
    return n * n;
}

template <typename T> inline constexpr
size_type<T> distance2(point2<T> const p, point2<T> const q) noexcept {
    static_assert(std::is_signed<T>::value, "TODO");

    return size_type<T>{static_cast<T>(
        square_of(value_cast(p.x) - value_cast(q.x))
      + square_of(value_cast(p.y) - value_cast(q.y)))};
}

template <typename T> inline constexpr
axis_aligned_rect<T> shrink_rect(axis_aligned_rect<T> const r) noexcept {
    return {offset_type_x<T> {r.x0 + 1}, offset_type_y<T> {r.y0 + 1}
          , offset_type_x<T> {r.x1 - 1}, offset_type_y<T> {r.y1 - 1}};
}

template <typename T> inline constexpr
axis_aligned_rect<T> grow_rect(axis_aligned_rect<T> const r) noexcept {
    return {offset_type_x<T> {r.x0 - 1}, offset_type_y<T> {r.y0 - 1}
          , offset_type_x<T> {r.x1 + 1}, offset_type_y<T> {r.y1 + 1}};
}

template <typename T> inline constexpr
axis_aligned_rect<T> move_to_origin(axis_aligned_rect<T> const r) noexcept {
    return r + (point2<T> {0, 0} - r.top_left());
}

template <typename T, typename U> inline constexpr
bool intersects(axis_aligned_rect<T> const& r, point2<U> const& p) noexcept {
    return (p.x >= r.x0)
        && (p.x <  r.x1)
        && (p.y >= r.y0)
        && (p.y <  r.y1);
}

template <typename T, typename U> inline constexpr
bool intersects(point2<U> const& p, axis_aligned_rect<T> const& r) noexcept {
    return intersects(r, p);
}

template <typename T, typename F>
void for_each_xy(axis_aligned_rect<T> const r, F f, std::integral_constant<int, 2>) {
    auto const x0 = value_cast(r.x0);
    auto const x1 = value_cast(r.x1);
    auto const y0 = value_cast(r.y0);
    auto const y1 = value_cast(r.y1);

    for (auto y = y0; y < y1; ++y) {
        bool const on_edge_y = (y == y0) || (y == y1 - 1);
        for (auto x = x0; x < x1; ++x) {
            bool const on_edge = on_edge_y || (x == x0) || (x == x1 - 1);
            f(point2<T> {x, y}, on_edge);
        }
    }
}

template <typename T, typename F>
void for_each_xy(axis_aligned_rect<T> const r, F f, std::integral_constant<int, 1>) {
    auto const x0 = value_cast(r.x0);
    auto const x1 = value_cast(r.x1);
    auto const y0 = value_cast(r.y0);
    auto const y1 = value_cast(r.y1);

    for (auto y = y0; y < y1; ++y) {
        for (auto x = x0; x < x1; ++x) {
            f(point2<T> {x, y});
        }
    }
}

template <typename T, typename F>
void for_each_xy(axis_aligned_rect<T> const r, F f) {
    constexpr int n = arity_of<F>::value;
    for_each_xy(r, f, std::integral_constant<int, n> {});
}

template <typename T, typename UnaryF>
void for_each_xy_edge(axis_aligned_rect<T> const r, UnaryF f)
    noexcept (noexcept(f(std::declval<point2<T>>())))
{
    if (value_cast(r.area()) == 1) {
        f(r.top_left());
        return;
    }

    auto const x0 = value_cast(r.x0);
    auto const x1 = value_cast(r.x1);
    auto const y0 = value_cast(r.y0);
    auto const y1 = value_cast(r.y1);

    //top edge
    for (auto x = x0; x < x1; ++x) {
        f(point2<T> {x, y0});
    }

    //left and right edge
    for (auto y = y0 + 1; y < y1 - 1; ++y) {
        f(point2<T> {x0,     y});
        f(point2<T> {x1 - 1, y});
    }

    // bottom edge
    for (auto x = x0; x < x1; ++x) {
        f(point2<T> {x, y1 - 1});
    }
}

//
// 1111111111
// 2000000002
// 2000000002
// 2000000002
// 3333333333
//
template <typename T, typename CenterF, typename EdgeF>
void for_each_xy_center_first(axis_aligned_rect<T> const r, CenterF center, EdgeF edge) {
    for_each_xy(shrink_rect(r), center);
    for_each_xy_edge(r, edge);
}

template <typename T, typename UnaryF>
void points_around(point2<T> const p, T const distance, UnaryF f)
    noexcept (noexcept(f(p)))
{
    auto const d = distance;
    auto const q = p - vec2<T> {d, d};
    auto const s = d * 2 + 1;

    size_type_x<T> const w = s;
    size_type_y<T> const h = s;
    axis_aligned_rect<T> r (q, w, h);

    for_each_xy_edge(r, f);
}

template <typename T>
inline constexpr bool rect_by_min_dimension(
    axis_aligned_rect<T> const a
  , axis_aligned_rect<T> const b
) noexcept {
    return (std::min(a.width(), a.height()) == std::min(b.width(), b.height()))
      ? a.area() < b.area()
      : std::min(a.width(), a.height()) < std::min(b.width(), b.height());
}

//
template <typename T>
inline constexpr T clamp(T const n, T const lo, T const hi) noexcept {
    return (n < lo)
      ? lo
      : (hi < n ? hi : n);
}

template <typename R, typename T>
inline constexpr R clamp_as(T const n, T const lo, T const hi) noexcept {
    //TODO this could be more intelligent
    return static_cast<R>(clamp(n, lo, hi));
}

template <typename R, typename T>
inline constexpr R clamp_as(T const n) noexcept {
    //TODO this could be more intelligent
    return static_cast<R>(clamp<T>(n, std::numeric_limits<R>::min()
                                    , std::numeric_limits<R>::max()));
}

template <typename T> inline constexpr
axis_aligned_rect<T> clamp(
    axis_aligned_rect<T> const r
  , axis_aligned_rect<T> const bounds
) noexcept {
    return {offset_type_x<T> {clamp(r.x0, bounds.x0, bounds.x1)}
          , offset_type_y<T> {clamp(r.y0, bounds.y0, bounds.y1)}
          , offset_type_x<T> {clamp(r.x1, bounds.x0, bounds.x1)}
          , offset_type_y<T> {clamp(r.y1, bounds.y0, bounds.y1)}};
}

//! type-casted ceil
template <typename R, typename T>
inline constexpr R ceil_as(T const n) noexcept {
    return static_cast<R>(std::ceil(n));
}

//! type-casted floor
template <typename R, typename T>
inline constexpr R floor_as(T const n) noexcept {
    return static_cast<R>(std::floor(n));
}

//! type-casted round
template <typename R, typename T>
inline constexpr R round_as(T const n) noexcept {
    return static_cast<R>(std::round(n));
}

} //namespace boken
