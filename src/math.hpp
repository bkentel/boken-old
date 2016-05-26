#pragma once

#include "math_types.hpp"
#include "functional.hpp"

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

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr T signof(T const n) noexcept {
    return (n < T {0}) ? T {-1}
         : (n > T {0}) ? T { 1}
                       : T { 0};
}

template <typename T, typename TagAxis, typename TagType>
basic_1_tuple<T, TagAxis, TagType>
signof(basic_1_tuple<T, TagAxis, TagType> const n) noexcept {
    return {signof(value_cast(n))};
}

template <typename T>
constexpr vec2<T> signof(vec2<T> const v) noexcept {
    return {signof(v.x), signof(v.y)};
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

template <typename T, typename TagType>
constexpr basic_2_tuple<T, TagType> clamp(axis_aligned_rect<T> const r, basic_2_tuple<T, TagType> const p) noexcept {
    return {
        clamp(value_cast(p.x), value_cast(r.x0), static_cast<T>(value_cast(r.x1) - 1))
      , clamp(value_cast(p.y), value_cast(r.y0), static_cast<T>(value_cast(r.y1) - 1))};
}

template <typename T, typename TagAxis, typename TagType>
constexpr basic_1_tuple<T, TagAxis, TagType>
abs(basic_1_tuple<T, TagAxis, TagType> const n) noexcept {
    return {value_cast(n) < T {0} ? -value_cast(n) : value_cast(n)};
}

template <typename T, typename TagType>
constexpr basic_2_tuple<T, TagType>
abs(basic_2_tuple<T, TagType> const n) noexcept {
    return {abs(n.x), abs(n.y)};
}

template <typename T, typename TagAxis, typename TagType>
constexpr basic_1_tuple<T, TagAxis, TagType>
floor(basic_1_tuple<T, TagAxis, TagType> const n) noexcept {
    return {std::floor(value_cast(n))};
}

template <typename T, typename TagType>
constexpr basic_2_tuple<T, TagType>
floor(basic_2_tuple<T, TagType> const n) noexcept {
    return {floor(n.x), floor(n.y)};
}

template <typename T, typename TagType>
constexpr auto magnitude_x(basic_2_tuple<T, TagType> const n) noexcept {
    return basic_1_tuple<T, tag_axis_x, tag_vector> {value_cast(n.x)};
}

template <typename T, typename TagType>
constexpr auto magnitude_y(basic_2_tuple<T, TagType> const n) noexcept {
    return basic_1_tuple<T, tag_axis_y, tag_vector> {value_cast(n.y)};
}

template <typename T, typename U, typename TagType>
constexpr auto make_2_tuple(
    basic_1_tuple<T, tag_axis_x, TagType> const x
  , basic_1_tuple<U, tag_axis_y, TagType> const y
) noexcept {
    return basic_2_tuple<safe_common_type_t<T, U, true>, TagType> {x, y};
}

namespace detail {

template <typename T>
constexpr bool is_cardinal_dir_impl(vec2<T> const v) noexcept {
    return v == vec2<T> {T {0}, T {1}}
        || v == vec2<T> {T {1}, T {0}};
}

} // namespace detail

template <typename T>
constexpr bool is_cardinal_dir(vec2<T> const v) noexcept {
    return detail::is_cardinal_dir_impl(boken::abs(v));
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

template <typename T, typename SetPixel>
void bresenham_line(T const x0, T const y0, T const x1, T const y1, SetPixel set_pixel) {
    static_assert(std::is_signed<T>::value, "");

    auto const f = void_as_bool<true>(set_pixel);

    T const dx = std::abs(x1 - x0);
    T const dy = std::abs(y1 - y0);
    T const sx = (x0 < x1) ? 1 : -1;
    T const sy = (y0 < y1) ? 1 : -1;
    T       x  = x0;
    T       y  = y0;

    for (T err = (dx > dy ? dx : -dy) / 2; ; ) {
        if (!f(x, y) || (x == x1 && y == y1)) {
            break;
        }

        T const prev_err = err;

        if (prev_err > -dx) { err -= dy; x += sx; }
        if (prev_err <  dy) { err += dx; y += sy; }
    }
}

template <typename T, typename SetPixel>
void bresenham_line(point2<T> const from, point2<T> const to, SetPixel set_pixel) {
    auto const f = void_as_bool<true>(set_pixel);
    bresenham_line(value_cast(from.x), value_cast(from.y)
                 , value_cast(to.x), value_cast(to.y)
                 , [&](T const x, T const y) {
                        return f(point2<T> {x, y});
                   });
}

} //namespace boken
