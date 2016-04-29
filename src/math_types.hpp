#pragma once

//
// Definitions and fundamental operations for mathematical types.
//

#include <type_traits>
#include <functional> //std::divides etc
#include <cstdint>
#include <cstddef>

namespace boken {

//=====--------------------------------------------------------------------=====
//                              Type traits
//=====--------------------------------------------------------------------=====

//! (safe) widening integral conversions
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

//! (safe) widening floating-point conversions
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

//! true for strictly (safe) widening arithmetic conversions
template <typename From, typename To>
struct is_safe_aithmetic_conversion : std::integral_constant<bool,
    (std::is_arithmetic<From>::value && std::is_arithmetic<To>::value)
    && (is_safe_integer_conversion<From, To>::value
       || is_safe_floating_point_conversion<From, To>::value)
>
{
};

//! a common (arithmetic) type between T and U that both types can be widened to.
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

//! For To = void -> From
//! Otherwise     -> To
template <typename From, typename To>
struct choose_result {
    using type = std::conditional_t<std::is_void<To>::value, From, To>;
};

template <typename From, typename To>
using choose_result_t = typename choose_result<From, To>::type;

//=====--------------------------------------------------------------------=====
//                                Tags
//=====--------------------------------------------------------------------=====
struct tag_axis_x    {};
struct tag_axis_y    {};
struct tag_axis_none {};
struct tag_point     {};
struct tag_vector    {};

//=====--------------------------------------------------------------------=====
//                          Forward declarations
//=====--------------------------------------------------------------------=====
template <typename T, typename Tag>
class tagged_value;

template <typename T, typename TagAxis, typename TagType>
class basic_1_tuple;

template <typename T, typename TagType>
class basic_2_tuple;

template <typename T>
class axis_aligned_rect;

//------------------------------------------------------------------------------
// value_cast
//------------------------------------------------------------------------------
//! Return the parameter safely widened to the @p To type if @p To is not void.
//! Otherwise return the underlying value of the parameter unchanged.

namespace detail {

template <typename T, typename Tag>
constexpr T get_value(tagged_value<T, Tag> const n) noexcept;

template <typename T, typename TagAxis, typename TagType>
constexpr T get_value(basic_1_tuple<T, TagAxis, TagType> const n) noexcept;

} //namespace detail

template <typename To = void
        , typename From
        , typename Result = choose_result_t<From, To>
        , typename = std::enable_if_t<std::is_arithmetic<From>::value>>
constexpr Result value_cast(From const n) noexcept {
    static_assert(is_safe_aithmetic_conversion<From, Result> {}, "");
    return static_cast<Result>(n);
}

template <typename To = void
        , typename From
        , typename Tag
        , typename Result = choose_result_t<From, To>>
constexpr Result value_cast(tagged_value<From, Tag> n) noexcept {
    return value_cast<Result>(detail::get_value(n));
}

template <typename To = void
        , typename From
        , typename TagAxis
        , typename TagType
        , typename Result = choose_result_t<From, To>>
constexpr Result value_cast(basic_1_tuple<From, TagAxis, TagType> n) noexcept {
    return value_cast<Result>(detail::get_value(n));
}

//=====--------------------------------------------------------------------=====
//                              Type aliases
//=====--------------------------------------------------------------------=====
template <typename T>
using size_type = basic_1_tuple<T, tag_axis_none, tag_vector>;

template <typename T>
using size_type_x = basic_1_tuple<T, tag_axis_x, tag_vector>;

template <typename T>
using size_type_y = basic_1_tuple<T, tag_axis_y, tag_vector>;

template <typename T>
using offset_type = basic_1_tuple<T, tag_axis_none, tag_point>;

template <typename T>
using offset_type_x = basic_1_tuple<T, tag_axis_x, tag_point>;

template <typename T>
using offset_type_y = basic_1_tuple<T, tag_axis_y, tag_point>;

template <typename T>
using point2 = basic_2_tuple<T, tag_point>;

template <typename T>
using vec2 = basic_2_tuple<T, tag_vector>;

using point2f = point2<float>;
using vec2f   = vec2<float>;
using sizef   = size_type<float>;
using sizefx  = size_type_x<float>;
using sizefy  = size_type_y<float>;
using offf    = offset_type<float>;
using offfx   = offset_type_x<float>;
using offfy   = offset_type_y<float>;

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

using rectf   = axis_aligned_rect<float>;
using recti32 = axis_aligned_rect<int32_t>;
using recti16 = axis_aligned_rect<int16_t>;

//=====--------------------------------------------------------------------=====
//                                Types
//=====--------------------------------------------------------------------=====
//! A tagged wrapper around a fundamental type.
template <typename T, typename Tag>
class tagged_value {
    static_assert(std::is_fundamental<T>::value, "");

    template <typename T0, typename Tag0>
    friend constexpr T0 detail::get_value(tagged_value<T0, Tag0>) noexcept;
public:
    using type = T;
    using tag  = Tag;

    constexpr tagged_value() = default;

    template <typename U>
    constexpr tagged_value(U const n) noexcept
      : value_ {value_cast<T>(n)}
    {
    }

    template <typename U>
    constexpr tagged_value(tagged_value<U, Tag> const n) noexcept
      : tagged_value {value_cast(n)}
    {
    }

    template <typename U>
    constexpr tagged_value& operator=(tagged_value<U, Tag> const n) noexcept {
        return (value_ = value_cast<T>(n)), *this;
    }
private:
    T value_ {};
};

struct identity_hash {
    template <typename T, typename Tag>
    constexpr size_t operator()(tagged_value<T, Tag> const id) const noexcept {
        return value_cast(id);
    }
};

//! 1 dimensional quantity
template <typename T, typename TagAxis, typename TagType>
class basic_1_tuple {
    static_assert(std::is_arithmetic<T>::value, "");

    template <typename T0, typename TagAxis0, typename TagType0>
    friend constexpr T0 detail::get_value(basic_1_tuple<T0, TagAxis0, TagType0>) noexcept;
public:
    using type     = T;
    using tag_axis = TagAxis;
    using tag_type = TagType;

    constexpr basic_1_tuple() noexcept = default;

    template <typename U>
    constexpr basic_1_tuple(U const n) noexcept
      : value_ {value_cast<T>(n)}
    {
        static_assert(is_safe_aithmetic_conversion<U, T>::value, "");
    }

    template <typename U>
    constexpr basic_1_tuple(basic_1_tuple<U, TagAxis, TagType> const n) noexcept
      : basic_1_tuple {value_cast(n)}
    {
    }

    template <typename U>
    constexpr basic_1_tuple& operator=(basic_1_tuple<U, TagAxis, TagType> const n) noexcept {
        static_assert(is_safe_aithmetic_conversion<U, T>::value, "");
        return (value_ = value_cast<T>(n)), *this;
    }
private:
    T value_ {};
};

//! 2 dimensional quantity
template <typename T, typename TagType>
class basic_2_tuple {
    static_assert(std::is_arithmetic<T>::value, "");
public:
    using type     = T;
    using tag_type = TagType;

    constexpr basic_2_tuple() noexcept = default;

    template <typename U, typename V>
    constexpr basic_2_tuple(U const xx, V const yy) noexcept
      : x {xx}, y {yy}
    {
        static_assert(is_safe_aithmetic_conversion<U, T>::value, "");
        static_assert(is_safe_aithmetic_conversion<V, T>::value, "");
    }

    template <typename U, typename V>
    constexpr basic_2_tuple(
        basic_1_tuple<U, tag_axis_x, TagType> const xx, V const yy
    ) noexcept
      : basic_2_tuple {value_cast(xx), value_cast(yy)}
    {
    }

    template <typename U, typename V>
    constexpr basic_2_tuple(
        U const xx, basic_1_tuple<V, tag_axis_y, TagType> const yy
    ) noexcept
      : basic_2_tuple {value_cast(xx), value_cast(yy)}
    {
    }

    template <typename U, typename V>
    constexpr basic_2_tuple(
        basic_1_tuple<U, tag_axis_x, TagType> const xx
      , basic_1_tuple<V, tag_axis_y, TagType> const yy
    ) noexcept
      : basic_2_tuple {value_cast(xx), value_cast(yy)}
    {
    }

    template <typename U>
    constexpr basic_2_tuple(basic_2_tuple<U, TagType> const p) noexcept
      : basic_2_tuple {p.x, p.y}
    {
    }

    template <typename U>
    constexpr basic_2_tuple& operator=(basic_2_tuple<U, TagType> const p) noexcept {
        static_assert(is_safe_aithmetic_conversion<U, T>::value, "");
        return (x = p.x), (y = p.y), *this;
    }

    basic_1_tuple<T, tag_axis_x, TagType> x {};
    basic_1_tuple<T, tag_axis_y, TagType> y {};
};

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
        static_assert(is_safe_aithmetic_conversion<U, T>::value, "");
        static_assert(is_safe_aithmetic_conversion<V, T>::value, "");
        static_assert(is_safe_aithmetic_conversion<W, T>::value, "");
        static_assert(is_safe_aithmetic_conversion<X, T>::value, "");
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

    constexpr point2<T> top_right() const noexcept {
        return {value_cast(x1), value_cast(y0)};
    }

    constexpr point2<T> bottom_left() const noexcept {
        return {value_cast(x0), value_cast(y1)};
    }

    constexpr point2<T> bottom_right() const noexcept {
        return {value_cast(x1), value_cast(y1)};
    }

    offset_type_x<T> x0 {};
    offset_type_y<T> y0 {};
    offset_type_x<T> x1 {};
    offset_type_y<T> y1 {};
};

//=====--------------------------------------------------------------------=====
//                                Functions
//=====--------------------------------------------------------------------=====

namespace detail {

template <typename T, typename Tag>
constexpr T get_value(tagged_value<T, Tag> const n) noexcept {
    return n.value_;
}

template <typename T, typename TagAxis, typename TagType>
constexpr T get_value(basic_1_tuple<T, TagAxis, TagType> const n) noexcept {
    return n.value_;
}

} //namespace detail

//------------------------------------------------------------------------------
// value_cast_unsafe
//------------------------------------------------------------------------------
//! Return the parameter unsafely narrowed to the @p To type.

template <typename To, typename From
        , typename = std::enable_if_t<std::is_arithmetic<From>::value>>
constexpr To value_cast_unsafe(From const n) noexcept {
    return static_cast<To>(n);
}

template <typename To, typename From, typename TagAxis, typename TagType>
constexpr To value_cast_unsafe(basic_1_tuple<From, TagAxis, TagType> const n) noexcept {
    return value_cast_unsafe<To>(value_cast(n));
}

//------------------------------------------------------------------------------
// underlying_cast_unsafe
//------------------------------------------------------------------------------

template <typename To, typename From, typename TagAxis, typename TagType>
constexpr basic_1_tuple<To, TagAxis, TagType>
underlying_cast_unsafe(basic_1_tuple<From, TagAxis, TagType> const n) noexcept {
    return {value_cast_unsafe<To>(n)};
}

template <typename To, typename From, typename TagType>
constexpr basic_2_tuple<To, TagType>
underlying_cast_unsafe(basic_2_tuple<From, TagType> const p) noexcept {
    return {value_cast_unsafe<To>(p.x), value_cast_unsafe<To>(p.y)};
}

template <typename To, typename From, typename TagType>
constexpr axis_aligned_rect<To>
underlying_cast_unsafe(axis_aligned_rect<From> const r) noexcept {
    return {value_cast_unsafe<To>(r.upper_left())
          , value_cast_unsafe<To>(r.bottom_right())};
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
    return detail::compute<TagAxis, tag_vector>(x, y, std::multiplies<> {});
}

//------------------------------------------------------------------------------
// size / size -> size
//------------------------------------------------------------------------------
template <typename T, typename U, typename TagAxis>
constexpr auto operator/(
    basic_1_tuple<T, TagAxis, tag_vector> const x
  , basic_1_tuple<U, TagAxis, tag_vector> const y
) noexcept {
    return detail::compute<TagAxis, tag_vector>(x, y, std::divides<> {});
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

template <typename T, typename U, typename Compare>
inline constexpr bool compare(T const x, U const y, Compare f) noexcept {
    using TX = std::decay_t<decltype(value_cast(x))>;
    using TY = std::decay_t<decltype(value_cast(y))>;

    using common_t = safe_common_type_t<TX, TY, true>;
    return f(value_cast<common_t>(x), value_cast<common_t>(y));
}

} //namespace detail

//------------------------------------------------------------------------------
// tagged_value
//------------------------------------------------------------------------------

template <typename T, typename U, typename Tag>
inline constexpr bool operator==(
    tagged_value<T, Tag> const x, tagged_value<U, Tag> const y
) noexcept {
    return detail::compare(x, y, std::equal_to<> {});
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator<(
    tagged_value<T, Tag> const x, tagged_value<U, Tag> const y
) noexcept {
    return detail::compare(x, y, std::less<> {});
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator<=(
    tagged_value<T, Tag> const x, tagged_value<U, Tag> const y
) noexcept {
    return detail::compare(x, y, std::less_equal<> {});
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator>(
    tagged_value<T, Tag> const x, tagged_value<U, Tag> const y
) noexcept {
    return detail::compare(x, y, std::greater<> {});
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator>=(
    tagged_value<T, Tag> const x, tagged_value<U, Tag> const y
) noexcept {
    return detail::compare(x, y, std::greater_equal<> {});
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator!=(
    tagged_value<T, Tag> const x, tagged_value<U, Tag> const y
) noexcept {
    return detail::compare(x, y, std::not_equal_to<> {});
}

template <typename T, typename Tag> inline constexpr
bool operator==(tagged_value<T, Tag> const a, std::nullptr_t) noexcept {
    return value_cast(a) == T {0};
}

template <typename T, typename Tag> inline constexpr
bool operator==(std::nullptr_t, tagged_value<T, Tag> const a) noexcept {
    return a == nullptr;
}

template <typename T, typename Tag> inline constexpr
bool operator!=(tagged_value<T, Tag> const a, std::nullptr_t) noexcept {
    return !(a == nullptr);
}

template <typename T, typename Tag> inline constexpr
bool operator!=(std::nullptr_t, tagged_value<T, Tag> const a) noexcept {
    return !(a == nullptr);
}

//------------------------------------------------------------------------------
// basic_1_tuple
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
// basic_2_tuple
//------------------------------------------------------------------------------

template <typename T, typename U, typename TagType>
inline constexpr bool operator==(
    basic_2_tuple<T, TagType> const p
  , basic_2_tuple<U, TagType> const q
) noexcept {
    return (p.x == q.x) && (p.y == q.y);
}

template <typename T, typename U, typename TagType>
inline constexpr bool operator!=(
    basic_2_tuple<T, TagType> const p
  , basic_2_tuple<U, TagType> const q
) noexcept {
    return !(p == q);
}

//------------------------------------------------------------------------------
// axis_aligned_rect
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
