#pragma once

#include "types.hpp"
#include "utility.hpp"

#include "math_types.hpp"

#include <type_traits>
#include <functional>
#include <cmath>
#include <cstdint>

namespace boken {

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
