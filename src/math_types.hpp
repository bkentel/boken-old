#pragma once

//
// Definitions for mathematical types. Operations on these types are defined
// elsewhere. This header should be included where a fully specified definition
// is required.
//

#include "math_forward.hpp"

#include <type_traits>
#include <cstdint>

namespace boken {

template <typename From, typename To>
struct is_safe_integer_conversion : std::false_type {};

template <> struct is_safe_integer_conversion< int8_t,   int8_t > : std::true_type {};
template <> struct is_safe_integer_conversion< int8_t,   int16_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint8_t,   int16_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int8_t,   int32_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint8_t,   int32_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int8_t,   int64_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint8_t,   int64_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int16_t,  int16_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int16_t,  int32_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint16_t,  int32_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int16_t,  int64_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint16_t,  int64_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int32_t,  int32_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int32_t,  int64_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint32_t,  int64_t> : std::true_type {};
template <> struct is_safe_integer_conversion< int64_t,  int64_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint8_t,  uint8_t > : std::true_type {};
template <> struct is_safe_integer_conversion<uint8_t,  uint16_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint8_t,  uint32_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint8_t,  uint64_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint16_t, uint16_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint16_t, uint32_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint16_t, uint64_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint32_t, uint32_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint32_t, uint64_t> : std::true_type {};
template <> struct is_safe_integer_conversion<uint64_t, uint64_t> : std::true_type {};

template <typename To, typename From, typename TagAxis, typename TagType, typename Result>
constexpr Result value_cast(basic_1_tuple<From, TagAxis, TagType> n) noexcept;

//------------------------------------------------------------------------------
//! 1 dimensional quantity
template <typename T, typename TagAxis, typename TagType>
class basic_1_tuple {
    static_assert(std::is_arithmetic<T>::value, "");

    template <typename To, typename From, typename TagAxis0, typename TagType0, typename Result>
    friend constexpr Result value_cast(basic_1_tuple<From, TagAxis0, TagType0>) noexcept;
public:
    using type     = T;
    using tag_axis = TagAxis;
    using tag_type = TagType;

    template <typename U, typename = std::enable_if_t<
        is_safe_integer_conversion<U, T>::value>>
    constexpr basic_1_tuple(U const n) noexcept
      : value_ {n}
    {
    }

    template <typename U, typename = std::enable_if_t<
        is_safe_integer_conversion<U, T>::value>>
    constexpr basic_1_tuple(basic_1_tuple<U, TagAxis, TagType> const n) noexcept
      : basic_1_tuple {value_cast(n)}
    {
    }

    template <typename U, typename = std::enable_if_t<
        is_safe_integer_conversion<U, T>::value>>
    constexpr basic_1_tuple& operator=(basic_1_tuple<U, TagAxis, TagType> const n) noexcept {
        return (value_ = value_cast(n)), *this;
    }
private:
    T value_;
};

//------------------------------------------------------------------------------
//! 2 dimensional quantity
template <typename T, typename TagType>
class basic_2_tuple {
    static_assert(std::is_arithmetic<T>::value, "");
public:
    using type = T;
    using tag_type = TagType;

    template <typename U, typename V, typename = std::enable_if_t<
        is_safe_integer_conversion<U, T>::value
     && is_safe_integer_conversion<V, T>::value>>
    constexpr basic_2_tuple(U const xx, V const yy) noexcept
      : x {xx}
      , y {yy}
    {
    }

    template <typename U, typename V, typename = std::enable_if_t<
        is_safe_integer_conversion<U, T>::value
     && is_safe_integer_conversion<V, T>::value>>
    constexpr basic_2_tuple(
        basic_1_tuple<U, tag_axis_x, TagType> const xx
      , basic_1_tuple<V, tag_axis_y, TagType> const yy
    ) noexcept
      : basic_2_tuple {value_cast(xx), value_cast(yy)}
    {
    }

    template <typename U, typename = std::enable_if_t<
        is_safe_integer_conversion<U, T>::value>>
    constexpr basic_2_tuple(
        basic_2_tuple<U, TagType> const p
    ) noexcept
      : basic_2_tuple {p.x, p.y}
    {
    }

    template <typename U, typename = std::enable_if_t<
        is_safe_integer_conversion<U, T>::value>>
    constexpr basic_2_tuple& operator=(
        basic_2_tuple<U, TagType> const p
    ) noexcept {
        return (x = p.x), (y = p.y), *this;
    }

    basic_1_tuple<T, tag_axis_x, TagType> x;
    basic_1_tuple<T, tag_axis_y, TagType> y;
};

//------------------------------------------------------------------------------
//! 2D axis-aligned rectangle
template <typename T>
class axis_aligned_rect {
    static_assert(std::is_arithmetic<T>::value, "");
public:
    using type = T;

    constexpr axis_aligned_rect() = default;

    template <typename U, typename V, typename W, typename X>
    constexpr axis_aligned_rect(
        offset_type_x<U> const left
      , offset_type_y<V> const top
      , offset_type_x<W> const right
      , offset_type_y<X> const bottom
    ) noexcept
      : axis_aligned_rect {
          value_cast(left),  value_cast(top)
        , value_cast(right), value_cast(bottom)}
    {
    }

    template <typename U, typename V, typename W>
    constexpr axis_aligned_rect(
        point2<U>      const p
      , size_type_x<V> const width
      , size_type_y<W> const height
    ) noexcept
      : axis_aligned_rect {
          value_cast(p.x)
        , value_cast(p.y)
        , value_cast(p.x) + value_cast(width)
        , value_cast(p.y) + value_cast(height)}
    {
    }

    template <typename U, typename V>
    constexpr axis_aligned_rect(point2<U> const p, point2<V> const q) noexcept
      : axis_aligned_rect {
          value_cast(p.x), value_cast(p.y)
        , value_cast(q.x), value_cast(q.y)}
    {
    }

    template <typename U, typename V, typename W, typename X>
    constexpr axis_aligned_rect(
        offset_type_x<U> const x
      , offset_type_y<V> const y
      , size_type_x<W>   const width
      , size_type_y<X>   const height
    ) noexcept
      : axis_aligned_rect {
          value_cast(x)
        , value_cast(y)
        , value_cast(x) + value_cast(width)
        , value_cast(y) + value_cast(height)}
    {
    }

    constexpr size_type_x<T> width()  const noexcept { return {value_cast(x1 - x0)}; }
    constexpr size_type_y<T> height() const noexcept { return {value_cast(y1 - y0)}; }

    constexpr size_type<T> area() const noexcept {
        return {value_cast(width()) * value_cast(height())};
    }

    constexpr point2<T> top_left() const noexcept {
        return {value_cast(x0), value_cast(y0)};
    }

    offset_type_x<T> x0 {};
    offset_type_y<T> y0 {};
    offset_type_x<T> x1 {};
    offset_type_y<T> y1 {};
private:
    template <typename U, typename V, typename W, typename X>
    constexpr axis_aligned_rect(
        U const x0_, V const y0_, W const x1_, X const y1_
    ) noexcept
      : x0 {x0_}
      , y0 {y0_}
      , x1 {x1_}
      , y1 {y1_}
    {
        static_assert(is_safe_integer_conversion<U, T>::value
                   && is_safe_integer_conversion<V, T>::value
                   && is_safe_integer_conversion<W, T>::value
                   && is_safe_integer_conversion<X, T>::value, "");
    }
};

} //namespace boken
