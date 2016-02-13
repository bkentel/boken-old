#pragma once

#include "types.hpp"
#include <type_traits>
#include <functional>
#include <cmath>
#include <cstdint>

namespace boken {

struct tag_point  {};
struct tag_vector {};

template <typename T, typename Tag>
struct basic_2_tuple {
    using tag = Tag;

    using type_x = std::conditional_t<
        std::is_same<Tag, tag_point>::value
      , offset_type_x<T>
      , size_type_x<T>>;

    using type_y = std::conditional_t<
        std::is_same<Tag, tag_point>::value
      , offset_type_y<T>
      , size_type_y<T>>;

    template <typename U>
    constexpr basic_2_tuple<U, Tag> cast_to() const noexcept {
        return basic_2_tuple<U, Tag> {static_cast<U>(value_cast(x))
                                    , static_cast<U>(value_cast(y))};
    }

    constexpr basic_2_tuple(T const x_, T const y_) noexcept
      : x {x_}, y {y_}
    {
    }

    constexpr basic_2_tuple(type_x const x_, type_y const y_) noexcept
      : x {x_}, y {y_}
    {
    }

    type_x x;
    type_y y;
};

template <typename T>
using point2  = basic_2_tuple<T, tag_point>;
using point2i = point2<int32_t>;
using point2f = point2<float>;

template <typename T>
using vec2  = basic_2_tuple<T, tag_vector>;
using vec2i = vec2<int32_t>;
using vec2f = vec2<float>;

namespace detail {

template <typename R, typename T, typename Tag0, typename Tag1, typename Op>
constexpr basic_2_tuple<T, R> do_op(basic_2_tuple<T, Tag0> const a, basic_2_tuple<T, Tag1> const b, Op const o) noexcept {
    return {static_cast<T>(o(value_cast(a.x), value_cast(b.x)))
          , static_cast<T>(o(value_cast(a.y), value_cast(b.y)))};
}

} //namespace detail

template <typename T> inline constexpr
point2<T> operator+(point2<T> const p, vec2<T> const v) noexcept {
    return detail::do_op<tag_point>(p, v, std::plus<> {});
}

template <typename T> inline constexpr
point2<T>& operator+=(point2<T>& p, vec2<T> const v) noexcept {
    return p = p + v;
}

template <typename T> inline constexpr
point2<T> operator-(point2<T> const p, vec2<T> const v) noexcept {
    return detail::do_op<tag_point>(p, v, std::minus<> {});
}

template <typename T> inline constexpr
vec2<T> operator-(vec2<T> const v, vec2<T> const w) noexcept {
    return detail::do_op<tag_vector>(v, w, std::minus<> {});
}

template <typename T> inline constexpr
vec2<T> operator-(point2<T> const p, point2<T> const q) noexcept {
    return detail::do_op<tag_vector>(p, q, std::minus<> {});
}

template <typename T> inline constexpr
point2<T>& operator-=(point2<T>& p, vec2<T> const v) noexcept {
    return p = p - v;
}

template <typename T, typename Tag> inline constexpr
basic_2_tuple<T, Tag> operator*(basic_2_tuple<T, Tag> const a, T const n) noexcept {
    return {a.x * n, a.y * n};
}

//! 2D axis-aligned rectangle (square).
template <typename T>
struct axis_aligned_rect {
    using type = T;

    static_assert(std::is_arithmetic<type>::value, "");

    constexpr axis_aligned_rect(
        offset_type_x<T> const left
      , offset_type_y<T> const top
      , offset_type_x<T> const right
      , offset_type_y<T> const bottom
    ) noexcept
      : x0 {value_cast(left)}
      , y0 {value_cast(top)}
      , x1 {value_cast(right)}
      , y1 {value_cast(bottom)}
    {
    }

    constexpr axis_aligned_rect(
        offset_type_x<T>  const x
      , offset_type_y<T>  const y
      , size_type_x<type> const width
      , size_type_y<type> const height
    ) noexcept
      : axis_aligned_rect {
            value_cast(x)
          , value_cast(y)
          , value_cast(x) + value_cast(width)
          , value_cast(y) + value_cast(height)}
    {
    }

    type x0 {};
    type y0 {};
    type x1 {};
    type y1 {};

    constexpr type width()  const noexcept { return x1 - x0; }
    constexpr type height() const noexcept { return y1 - y0; }
    constexpr type area()   const noexcept { return width() * height(); }
private:
    constexpr axis_aligned_rect(
        type const x0_, type const y0_
      , type const x1_, type const y1_
    ) noexcept
      : x0 {x0_}
      , y0 {y0_}
      , x1 {x1_}
      , y1 {y1_}
    {
    }
};

//
template <typename T>
inline constexpr T clamp(T const n, T const lo, T const hi) noexcept {
    return (n < lo)
      ? lo
      : (hi < n ? hi : n);
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
