#pragma once

#include <type_traits> // conditional_t etc
#include <cstddef>     // ptrdiff_t
#include <cstdint>

namespace boken {

//===------------------------------------------------------------------------===
//                          Forward declarations
//===------------------------------------------------------------------------===
template <typename T, typename Tag>
class tagged_integral_value;

template <typename Test, typename IfNotVoid>
using choose_void_t = std::conditional_t<
    std::is_void<Test>::value, IfNotVoid, Test>;

template <
    typename DesiredResultType = void
  , typename ValueType
  , typename Tag
  , typename ResultType = choose_void_t<DesiredResultType, ValueType>>
constexpr ResultType value_cast(
    tagged_integral_value<ValueType, Tag> n) noexcept;

template <
    typename DesiredResultType = void
  , typename ValueType
  , typename Tag
  , typename ResultType = choose_void_t<DesiredResultType, ValueType>>
constexpr ResultType value_cast(ValueType n) noexcept;

//===------------------------------------------------------------------------===
//                          Definitions
//===------------------------------------------------------------------------===

//! A tagged wrapper around an integral data type.
template <typename T, typename Tag>
class tagged_integral_value {
    template <typename U, typename V, typename W, typename X>
    friend constexpr X boken::value_cast(tagged_integral_value<V, W>) noexcept;
public:
    using type = std::decay_t<T>;
    using tag  = Tag;

    static_assert(std::is_integral<type>::value, "");

    tagged_integral_value() = default;

    template <typename U, typename = std::enable_if_t<
        std::is_same<std::common_type_t<type, U>, type>::value>>
    constexpr explicit tagged_integral_value(U const n) noexcept
      : value_ {n}
    {
    }
private:
    T value_ {};
};

//! The means of getting the underlying value out of a tagged integral type.
template <
    typename DesiredResultType
  , typename ValueType
  , typename Tag
  , typename ResultType>
constexpr ResultType value_cast(
    tagged_integral_value<ValueType, Tag> const n
) noexcept {
    return static_cast<ResultType>(n.value_);
}

template <
    typename DesiredResultType
  , typename ValueType
  , typename Tag
  , typename ResultType>
constexpr ResultType value_cast(ValueType const n) noexcept {
    return static_cast<ResultType>(n);
}

template <typename T, typename U, typename Tag>
inline constexpr auto operator+(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return tagged_integral_value<std::common_type_t<T, U>, Tag> {
        value_cast(a) + value_cast(b)};
}

template <typename T, typename U, typename Tag>
inline constexpr auto operator-(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return tagged_integral_value<std::common_type_t<T, U>, Tag> {
        value_cast(a) - value_cast(b)};
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator<(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return value_cast(a) < value_cast(b);
}

//===------------------------------------------------------------------------===
//                                  Types
//===------------------------------------------------------------------------===

struct tag_size_type   {};
struct tag_area_type   {};
struct tag_offset_type {};

struct tag_size_type_x {};
struct tag_size_type_y {};

struct tag_offset_type_x {};
struct tag_offset_type_y {};

template <typename T>
using size_type = tagged_integral_value<T, tag_size_type>;

template <typename T>
using size_type_x = tagged_integral_value<T, tag_size_type_x>;

template <typename T>
using size_type_y = tagged_integral_value<T, tag_size_type_y>;

template <typename T>
using offset_type = tagged_integral_value<T, tag_offset_type>;

template <typename T>
using offset_type_x = tagged_integral_value<T, tag_offset_type_x>;

template <typename T>
using offset_type_y = tagged_integral_value<T, tag_offset_type_y>;


// size is used by the standard, so "sz" it is.
template <typename T>
inline constexpr size_type<T> sz(T const n) noexcept {
    return size_type<T> {n};
}

} //namespace boken
