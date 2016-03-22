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

template <typename From, typename To>
struct is_safe_floating_point_conversion : std::false_type {};

template <> struct is_safe_floating_point_conversion< int8_t,  float> : std::true_type {};
template <> struct is_safe_floating_point_conversion< int16_t, float> : std::true_type {};
template <> struct is_safe_floating_point_conversion<uint8_t,  float> : std::true_type {};
template <> struct is_safe_floating_point_conversion<uint16_t, float> : std::true_type {};
template <> struct is_safe_floating_point_conversion<float,    float> : std::true_type {};

template <> struct is_safe_floating_point_conversion< int8_t,  double> : std::true_type {};
template <> struct is_safe_floating_point_conversion< int16_t, double> : std::true_type {};
template <> struct is_safe_floating_point_conversion< int32_t, double> : std::true_type {};
template <> struct is_safe_floating_point_conversion<uint8_t,  double> : std::true_type {};
template <> struct is_safe_floating_point_conversion<uint16_t, double> : std::true_type {};
template <> struct is_safe_floating_point_conversion<uint32_t, double> : std::true_type {};
template <> struct is_safe_floating_point_conversion<float,    double> : std::true_type {};
template <> struct is_safe_floating_point_conversion<double,   double> : std::true_type {};

template <typename From, typename To>
struct is_safe_aithmetic_conversion : std::integral_constant<bool,
    is_safe_integer_conversion<From, To>::value
 || is_safe_floating_point_conversion<From, To>::value>
{
};

template <typename T, typename U, bool Check = false>
struct safe_common_type {
    static constexpr bool value =
        is_safe_aithmetic_conversion<T, U>::value
     || is_safe_aithmetic_conversion<U, T>::value;

    static_assert(!Check || value, "");

    using type = std::conditional_t<
        value, std::common_type_t<T, U>, void>;
};

template <typename T, typename U, bool Check = false>
using safe_common_type_t = typename safe_common_type<T, U, Check>::type;

template <typename To, typename From, typename TagAxis, typename TagType, typename Result>
constexpr Result value_cast(basic_1_tuple<From, TagAxis, TagType> n) noexcept;

template <
    typename To = void
  , typename From
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>
  , typename = std::enable_if_t<std::is_arithmetic<From>::value>>
constexpr Result value_cast(From const n) noexcept {
    static_assert(std::is_arithmetic<From> {}, "");
    static_assert(std::is_arithmetic<Result> {}, "");
    static_assert(is_safe_aithmetic_conversion<From, Result> {}, "");
    return static_cast<Result>(n);
}

template <
    typename To = void
  , typename From
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>
  , typename = std::enable_if_t<std::is_arithmetic<From>::value>>
constexpr Result value_cast_unsafe(From const n) noexcept {
    return static_cast<Result>(n);
}

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

    constexpr basic_1_tuple() noexcept = default;

    template <typename U>
    constexpr basic_1_tuple(U const n) noexcept
      : value_ {value_cast<T>(n)}
    {
    }

    template <typename U>
    constexpr basic_1_tuple(basic_1_tuple<U, TagAxis, TagType> const n) noexcept
      : value_ {value_cast<T>(n)}
    {
    }

    template <typename U>
    constexpr basic_1_tuple& operator=(basic_1_tuple<U, TagAxis, TagType> const n) noexcept {
        return (value_ = value_cast<T>(n)), *this;
    }
private:
    T value_ {};
};

//------------------------------------------------------------------------------
//! 2 dimensional quantity
template <typename T, typename TagType>
class basic_2_tuple {
    static_assert(std::is_arithmetic<T>::value, "");
public:
    using type = T;
    using tag_type = TagType;

    constexpr basic_2_tuple() noexcept = default;

    template <typename U, typename V>
    constexpr basic_2_tuple(U const xx, V const yy) noexcept
      : x {xx}, y {yy}
    {
    }

    template <typename U>
    constexpr basic_2_tuple(basic_2_tuple<U, TagType> const p) noexcept
      : basic_2_tuple {p.x, p.y}
    {
    }

    template <typename U>
    constexpr basic_2_tuple& operator=(basic_2_tuple<U, TagType> const p) noexcept {
        return (x = p.x), (y = p.y), *this;
    }

    basic_1_tuple<T, tag_axis_x, TagType> x {};
    basic_1_tuple<T, tag_axis_y, TagType> y {};
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
      : x0 {left}, y0 {top}, x1 {right}, y1 {bottom}
    {
    }

    template <typename U, typename V, typename W>
    constexpr axis_aligned_rect(
        point2<U>      const p
      , size_type_x<V> const width
      , size_type_y<W> const height
    ) noexcept
      : axis_aligned_rect {p.x, p.y, p.x + width, p.y + height}
    {
    }

    template <typename U, typename V>
    constexpr axis_aligned_rect(point2<U> const p, point2<V> const q) noexcept
      : axis_aligned_rect {p.x, p.y, q.x, q.y}
    {
    }

    template <typename U, typename V, typename W, typename X>
    constexpr axis_aligned_rect(
        offset_type_x<U> const x
      , offset_type_y<V> const y
      , size_type_x<W>   const width
      , size_type_y<X>   const height
    ) noexcept
      : axis_aligned_rect {x, y, x + width, y + height}
    {
    }

    constexpr size_type_x<T> width()  const noexcept { return x1 - x0; }
    constexpr size_type_y<T> height() const noexcept { return y1 - y0; }

    constexpr size_type<T> area() const noexcept {
        return {value_cast(width()) * value_cast(height())};
    }

    constexpr point2<T> top_left() const noexcept {
        return {value_cast(x0), value_cast(y0)};
    }

    constexpr point2<T> bottom_right() const noexcept {
        return {value_cast(x1), value_cast(y1)};
    }

    offset_type_x<T> x0 {};
    offset_type_y<T> y0 {};
    offset_type_x<T> x1 {};
    offset_type_y<T> y1 {};
};

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
    return value_cast<Result>(n.value_);
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
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>>
constexpr Result value_cast_unsafe(basic_1_tuple<From, TagAxis, TagType> const n) noexcept {
    return value_cast_unsafe<Result>(value_cast(n));
}

//------------------------------------------------------------------------------
template <
    typename To = void
  , typename From
  , typename TagAxis
  , typename TagType
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>>
constexpr auto underlying_cast_unsafe(basic_1_tuple<From, TagAxis, TagType> const n) noexcept {
    return basic_1_tuple<Result, TagAxis, TagType> {
        value_cast_unsafe<Result>(n)};
}

//------------------------------------------------------------------------------
template <
    typename To = void
  , typename From
  , typename TagType
  , typename Result = std::conditional_t<std::is_void<To>::value, From, To>>
constexpr auto underlying_cast_unsafe(basic_2_tuple<From, TagType> const p) noexcept {
    return basic_2_tuple<Result, TagType> {
        value_cast_unsafe<Result>(p.x), value_cast_unsafe<Result>(p.y)};
}

//=====--------------------------------------------------------------------=====
//                           Arithmetic Operations
//=====--------------------------------------------------------------------=====

//------------------------------------------------------------------------------
namespace detail {

template <
    typename ResultAxis
  , typename ResultType
  , typename T, typename U
  , typename Compute>
inline constexpr auto compute_scalar(T const x, U const y, Compute f) noexcept {
    using common_t = safe_common_type_t<T, U, true>;
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

//------------------------------------------------------------------------------
// scale by a constant
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis, typename TagType
  , typename = std::enable_if_t<std::is_arithmetic<U>::value>>
constexpr auto operator*(
    basic_1_tuple<T, TagAxis, TagType> const x, U const c
) noexcept {
    return detail::compute<TagAxis, TagType>(x, c, std::multiplies<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType
  , typename = std::enable_if_t<std::is_arithmetic<U>::value>>
constexpr auto operator*(
    U const c, basic_1_tuple<T, TagAxis, TagType> const x
) noexcept {
    return x * c;
}

template <typename T, typename U, typename TagAxis, typename TagType
  , typename = std::enable_if_t<std::is_arithmetic<U>::value>>
constexpr basic_1_tuple<T, TagAxis, TagType>& operator*=(
    basic_1_tuple<T, TagAxis, TagType>& x, U const c
) noexcept {
    return (x = x * c);
}

template <typename T, typename U, typename TagAxis, typename TagType
  , typename = std::enable_if_t<std::is_arithmetic<U>::value>>
constexpr auto operator/(
    basic_1_tuple<T, TagAxis, TagType> const x, U const c
) noexcept {
    return detail::compute<TagAxis, TagType>(x, c, std::divides<> {});
}

template <typename T, typename U, typename TagAxis, typename TagType
  , typename = std::enable_if_t<std::is_arithmetic<U>::value>>
constexpr auto operator/(
    U const c, basic_1_tuple<T, TagAxis, TagType> const x
) noexcept {
    return x / c;
}

template <typename T, typename U, typename TagAxis, typename TagType
  , typename = std::enable_if_t<std::is_arithmetic<U>::value>>
constexpr basic_1_tuple<T, TagAxis, TagType>& operator/=(
    basic_1_tuple<T, TagAxis, TagType>& x, U const c
) noexcept {
    return (x = x / c);
}

//------------------------------------------------------------------------------
// size * size -> size
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator*(
    basic_1_tuple<T, TagAxis, tag_vector> const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, TagType>(x, y, std::multiplies<> {});
}

//------------------------------------------------------------------------------
// size / size -> size
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator/(
    basic_1_tuple<T, TagAxis, tag_vector> const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, TagType>(x, y, std::divides<> {});
}

//------------------------------------------------------------------------------
// size + size -> size
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator+(
    basic_1_tuple<T, TagAxis, tag_vector> const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, tag_vector>(x, y, std::plus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_vector>& operator+=(
    basic_1_tuple<T, TagAxis, tag_vector>&      x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x + y);
}

//------------------------------------------------------------------------------
// offset + size -> offset
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator+(
    basic_1_tuple<T, TagAxis, tag_point>  const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, tag_point>(x, y, std::plus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_point>& operator+=(
    basic_1_tuple<T, TagAxis, tag_point>&       x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x + y);
}

//------------------------------------------------------------------------------
// size - size -> size
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator-(
    basic_1_tuple<T, TagAxis, tag_vector> const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, tag_vector>(x, y, std::minus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_vector>& operator-=(
    basic_1_tuple<T, TagAxis, tag_vector>&      x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x - y);
}

//------------------------------------------------------------------------------
// offset - size -> offset
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator-(
    basic_1_tuple<T, TagAxis, tag_point>  const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, tag_point>(x, y, std::minus<> {});
}

template <typename T, typename U, typename TagAxis>
constexpr basic_1_tuple<T, TagAxis, tag_point>& operator-=(
    basic_1_tuple<T, TagAxis, tag_point>&       x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return (x = x - y);
}

//------------------------------------------------------------------------------
// offset - offset -> size
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator-(
    basic_1_tuple<T, TagAxis, tag_point> const x
  , basic_1_tuple<U, TagAxis, tag_point> const y
) noexcept {
    return detail::compute<TagAxis, tag_vector>(x, y, std::minus<> {});
}

//------------------------------------------------------------------------------
// scale by a constant
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagType>
constexpr auto operator*(basic_2_tuple<T, TagType> const p, U const c) noexcept {
    return basic_2_tuple<safe_common_type_t<T, U, true>, TagType> {
        detail::compute<tag_axis_x, TagType>(p.x, c, std::multiplies<> {})
      , detail::compute<tag_axis_y, TagType>(p.y, c, std::multiplies<> {})};
}

template <typename T, typename U, typename TagType>
constexpr auto operator*(U const c, basic_2_tuple<T, TagType> const p) noexcept {
    return p * c;
}

template <typename T, typename U, typename TagType>
constexpr auto& operator*=(basic_2_tuple<T, TagType>& p, U const c) noexcept {
    return (p = p * c);
}

template <typename T, typename U, typename TagType>
constexpr auto operator/(basic_2_tuple<T, TagType> const p, U const c) noexcept {
    return basic_2_tuple<safe_common_type_t<T, U, true>, TagType> {
        detail::compute<tag_axis_x, TagType>(p.x, c, std::divides<> {})
      , detail::compute<tag_axis_y, TagType>(p.y, c, std::divides<> {})};
}

template <typename T, typename U, typename TagType>
constexpr auto operator/(U const c, basic_2_tuple<T, TagType> const p) noexcept {
    return p / c;
}

template <typename T, typename U, typename TagType>
constexpr auto& operator/=(basic_2_tuple<T, TagType>& p, U const c) noexcept {
    return (p = p / c);
}

//------------------------------------------------------------------------------
// point + point -> vector
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// vector + vector -> vector
//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator+(vec2<T> const u, vec2<U> const v) noexcept {
    return vec2<safe_common_type_t<T, U, true>> {u.x + v.x, u.y + v.y};
}

template <typename T, typename U>
constexpr vec2<T>& operator+=(vec2<T>& u, vec2<U> const v) noexcept {
    return (u = u + v);
}

//------------------------------------------------------------------------------
// vector - vector -> vector
//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator-(vec2<T> const u, vec2<U> const v) noexcept {
    return vec2<safe_common_type_t<T, U, true>> {u.x - v.x, u.y - v.y};
}

template <typename T, typename U>
constexpr vec2<T>& operator-=(vec2<T>& u, vec2<U> const v) noexcept {
    return (u = u - v);
}

//------------------------------------------------------------------------------
// point + vector -> point
//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator+(point2<T> const p, vec2<U> const v) noexcept {
    return point2<safe_common_type_t<T, U, true>> {p.x + v.x, p.y + v.y};
}

template <typename T, typename U>
constexpr point2<T>& operator+=(point2<T>& p, vec2<U> const v) noexcept {
    return (p = p + v);
}

//------------------------------------------------------------------------------
// point - vector -> point
//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator-(point2<T> const p, vec2<U> const v) noexcept {
    return point2<safe_common_type_t<T, U, true>> {p.x - v.x, p.y - v.y};
}

template <typename T, typename U>
constexpr point2<T>& operator-=(point2<T>& p, vec2<U> const v) noexcept {
    return (p = p - v);
}

//------------------------------------------------------------------------------
// point - point -> vector
//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator-(point2<T> const p, point2<U> const q) noexcept {
    return vec2<safe_common_type_t<T, U, true>> {p.x - q.x, p.y - q.y};
}

//------------------------------------------------------------------------------
// rect + vector -> rect
//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator+(axis_aligned_rect<T> const r, vec2<U> const v) noexcept {
    return axis_aligned_rect<safe_common_type_t<T, U, true>> {
        r.x0 + v.x, r.y0 + v.y
      , r.x1 + v.x, r.y1 + v.y
    };
}

template <typename T, typename U>
constexpr axis_aligned_rect<T> operator+=(axis_aligned_rect<T>& r, vec2<U> const v) noexcept {
    return (r = r + v);
}

//------------------------------------------------------------------------------
// rect - vector -> rect
//------------------------------------------------------------------------------
template <typename T, typename U>
constexpr auto operator-(axis_aligned_rect<T> const r, vec2<U> const v) noexcept {
    return axis_aligned_rect<safe_common_type_t<T, U, true>> {
        r.x0 - v.x, r.y0 - v.y
      , r.x1 - v.x, r.y1 - v.y
    };
}

template <typename T, typename U>
constexpr axis_aligned_rect<T> operator-=(axis_aligned_rect<T>& r, vec2<U> const v) noexcept {
    return (r = r +-v);
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
    using common_t = safe_common_type_t<T, U, true>;
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

} //namespace boken
