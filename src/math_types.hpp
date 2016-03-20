#pragma once

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

struct tag_axis_x    {};
struct tag_axis_y    {};
struct tag_axis_none {};
struct tag_point     {};
struct tag_vector    {};

template <typename T, typename TagAxis, typename TagType>
class basic_1_tuple;

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

//=====--------------------------------------------------------------------=====
//                                Type Aliases
//=====--------------------------------------------------------------------=====
template <typename T>
using size_type = basic_1_tuple<T, tag_axis_none, tag_point>;

template <typename T>
using size_type_x = basic_1_tuple<T, tag_axis_x, tag_point>;

template <typename T>
using size_type_y = basic_1_tuple<T, tag_axis_y, tag_point>;

template <typename T>
using offset_type = basic_1_tuple<T, tag_axis_none, tag_vector>;

template <typename T>
using offset_type_x = basic_1_tuple<T, tag_axis_x, tag_vector>;

template <typename T>
using offset_type_y = basic_1_tuple<T, tag_axis_y, tag_vector>;

template <typename T>
using point2 = basic_2_tuple<T, tag_point>;

template <typename T>
using vec2 = basic_2_tuple<T, tag_vector>;

using point2i32 = point2<int32_t>;
using vec2i32   = vec2<int32_t>;
using sizei32   = size_type<int32_t>;
using sizei32x  = size_type_x<int32_t>;
using sizei32y  = size_type_y<int32_t>;
using offi32    = offset_type<int32_t>;
using offi32x   = offset_type_x<int32_t>;
using offi32y   = offset_type_y<int32_t>;

using point2i16 = point2<int16_t>;
using vec2i16   = vec2<int16_t>;
using sizei16   = size_type<int16_t>;
using sizei16x  = size_type_x<int16_t>;
using sizei16y  = size_type_y<int16_t>;
using offi16    = offset_type<int16_t>;
using offi16x   = offset_type_x<int16_t>;
using offi16y   = offset_type_y<int16_t>;

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
          value_cast(left)
        , value_cast(top)
        , value_cast(right)
        , value_cast(bottom)}
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

    constexpr size_type_x<T> width()  const noexcept { return x1 - x0; }
    constexpr size_type_y<T> height() const noexcept { return y1 - y0; }

    constexpr size_type<T> area() const noexcept {
        return {value_cast(width()) * value_cast(height())};
    }

    constexpr point2<T> top_left() const noexcept { return {x0, y0}; }

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

//=====--------------------------------------------------------------------=====
//                                Type Aliases
//=====--------------------------------------------------------------------=====
using recti32 = axis_aligned_rect<int32_t>;
using recti16 = axis_aligned_rect<int16_t>;

} //namespace boken
