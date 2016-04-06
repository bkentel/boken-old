#pragma once

#include "math_types.hpp"

#include <type_traits>
#include <functional>
#include <limits>
#include <cmath>
#include <cstdint>

namespace boken {

// unsigned -> unsigned
template <typename To, typename From>
constexpr bool is_in_range(From const n, std::integral_constant<int, 0>) noexcept {
    return n >= std::numeric_limits<To>::min()
        && n <= std::numeric_limits<To>::max();
}

// signed -> unsigned
template <typename To, typename From>
constexpr bool is_in_range(From const n, std::integral_constant<int, 1>) noexcept {
    return n >= From {0} && n <= std::numeric_limits<To>::max();
}

// unsigned -> signed
template <typename To, typename From>
constexpr bool is_in_range(From const n, std::integral_constant<int, 2>) noexcept {
    return n <= std::numeric_limits<To>::max();
}

// signed -> signed
template <typename To, typename From>
constexpr bool is_in_range(From const n, std::integral_constant<int, 3>) noexcept {
    return n >= std::numeric_limits<To>::min()
        && n <= std::numeric_limits<To>::max();
}

template <typename To, typename From>
constexpr bool is_in_range(From const n) noexcept {
    static_assert(std::is_arithmetic<From>::value, "");
    static_assert(std::is_arithmetic<To>::value, "");
    return is_safe_aithmetic_conversion<From, To>::value
        || is_in_range<To>(n,std::integral_constant<int,
            std::is_signed<From> {} ? 0b01 : 0
          | std::is_signed<To>   {} ? 0b10 : 0> {});
}

template <typename T, typename U, typename TagAxis, typename TagType> inline constexpr
auto min(
    basic_1_tuple<T, TagAxis, TagType> const x
  , basic_1_tuple<U, TagAxis, TagType> const y
) noexcept {
    return x < y ? x : y;
}

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

template <typename T>
inline constexpr T min_dimension(axis_aligned_rect<T> const r) noexcept {
    return std::min(value_cast(r.width()), value_cast(r.height()));
}

template <typename T>
inline constexpr bool rect_by_min_dimension(
    axis_aligned_rect<T> const a
  , axis_aligned_rect<T> const b
) noexcept {
    return (min_dimension(a) == min_dimension(b))
      ? a.area() < b.area()
      : min_dimension(a) < min_dimension(b);
}

//
template <typename T, typename U, typename V, typename = std::enable_if_t<
    std::is_arithmetic<T>::value
 && std::is_arithmetic<U>::value
 && std::is_arithmetic<V>::value>>
inline constexpr T clamp(T const n, U const lo, V const hi) noexcept {
    static_assert(is_safe_aithmetic_conversion<U, T> {}, "");
    static_assert(is_safe_aithmetic_conversion<V, T> {}, "");

    return (n < lo)
      ? lo
      : (hi < n ? hi : n);
}

//
template <typename T, typename U, typename V, typename TagAxis, typename TagType>
inline constexpr basic_1_tuple<T, TagAxis, TagType> clamp(
    basic_1_tuple<T, TagAxis, TagType> const n
  , basic_1_tuple<U, TagAxis, TagType> const lo
  , basic_1_tuple<V, TagAxis, TagType> const hi
) noexcept {
    return clamp(value_cast(n), value_cast(lo), value_cast(hi));
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

template <typename T, typename TagAxis, typename TagType>
constexpr basic_1_tuple<T, TagAxis, TagType>
abs(basic_1_tuple<T, TagAxis, TagType> const n) noexcept {
    return {value_cast(n) < T {0} ? -value_cast(n) : value_cast(n)};
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
