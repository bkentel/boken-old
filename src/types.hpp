#pragma once

#include <type_traits> // conditional_t etc
#include <memory>
#include <cstddef>     // ptrdiff_t
#include <cstdint>

namespace boken { class world; }

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

namespace detail {
template <typename T, typename Tag>
constexpr T value_cast_impl(tagged_integral_value<T, Tag> n) noexcept;
} //namespace detail

template <
    typename DesiredResultType = void
  , typename ValueType
  , typename ResultType = choose_void_t<DesiredResultType, ValueType>>
constexpr ResultType value_cast(ValueType n) noexcept;

//===------------------------------------------------------------------------===
//                          Definitions
//===------------------------------------------------------------------------===

//! A tagged wrapper around an integral data type.
template <typename T, typename Tag>
class tagged_integral_value {
    template <typename T1, typename Tag1>
    friend constexpr T1 detail::value_cast_impl(tagged_integral_value<T1, Tag1>) noexcept;
public:
    using type = std::decay_t<T>;
    using tag  = Tag;

    static_assert(std::is_arithmetic<type>::value, "");

    constexpr tagged_integral_value() = default;
    constexpr tagged_integral_value(tagged_integral_value const&) = default;
    constexpr tagged_integral_value(tagged_integral_value&&) = default;
    tagged_integral_value& operator=(tagged_integral_value const&) = default;
    tagged_integral_value& operator=(tagged_integral_value&&) = default;

    template <typename U, typename =
        std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr explicit tagged_integral_value(U const n) noexcept
      : value_ {static_cast<T>(n)}
    {
        // TODO unsafe conversion could happen here
        //static_assert(std::is_convertible<U, T>::value, "");
    }
private:
    T value_ {};
};

namespace detail {
template <typename T, typename Tag>
constexpr T value_cast_impl(tagged_integral_value<T, Tag> const n) noexcept {
    return n.value_;
}
} //namespace detail

//! The means of getting the underlying value out of a tagged integral type.
template <
    typename DesiredResultType
  , typename ValueType
  , typename Tag
  , typename ResultType>
constexpr ResultType value_cast(
    tagged_integral_value<ValueType, Tag> const n
) noexcept {
    return static_cast<ResultType>(detail::value_cast_impl(n));
}

template <
    typename DesiredResultType
  , typename ValueType
  , typename ResultType>
constexpr ResultType value_cast(ValueType const n) noexcept {
    return static_cast<ResultType>(n);
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator>(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return value_cast(a) > value_cast(b);
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator<(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return value_cast(a) < value_cast(b);
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator<=(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return value_cast(a) <= value_cast(b);
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator==(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return value_cast(a) == value_cast(b);
}

template <typename T, typename Tag> inline constexpr
bool operator==(tagged_integral_value<T, Tag> const a, std::nullptr_t) noexcept {
    return value_cast(a) == 0;
}

template <typename T, typename Tag> inline constexpr
bool operator==(std::nullptr_t, tagged_integral_value<T, Tag> const a) noexcept {
    return a == nullptr;
}

template <typename T, typename U, typename Tag>
inline constexpr bool operator!=(
    tagged_integral_value<T, Tag> const a
  , tagged_integral_value<U, Tag> const b
) noexcept {
    return !(a == b);
}

template <typename T, typename Tag> inline constexpr
bool operator!=(tagged_integral_value<T, Tag> const a, std::nullptr_t) noexcept {
    return !(a == nullptr);
}

template <typename T, typename Tag> inline constexpr
bool operator!=(std::nullptr_t, tagged_integral_value<T, Tag> const a) noexcept {
    return !(a == nullptr);
}

//===------------------------------------------------------------------------===
//                                  Types
//===------------------------------------------------------------------------===

struct tag_size_type   {};
struct tag_area_type   {};
struct tag_offset_type {};

struct tag_size_type_x : tag_size_type {};
struct tag_size_type_y : tag_size_type {};

struct tag_offset_type_x : tag_offset_type {};
struct tag_offset_type_y : tag_offset_type {};

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

using sizei  = size_type<int32_t>;
using sizeix = size_type_x<int32_t>;
using sizeiy = size_type_y<int32_t>;
using offi   = offset_type<int32_t>;
using offix  = offset_type_x<int32_t>;
using offiy  = offset_type_y<int32_t>;

struct tag_id_entity {};
struct tag_id_instance_entity {};

struct tag_id_item {};
struct tag_id_instance_item {};

using entity_id          = tagged_integral_value<uint32_t, tag_id_entity>;
using entity_instance_id = tagged_integral_value<uint32_t, tag_id_instance_entity>;
using item_id            = tagged_integral_value<uint32_t, tag_id_item>;
using item_instance_id   = tagged_integral_value<uint32_t, tag_id_instance_item>;

// size is used by the standard, so "sz" it is.
template <typename T>
inline constexpr size_type<T> sz(T const n) noexcept {
    return size_type<T> {n};
}

class item_deleter {
public:
    using pointer = item_instance_id;

    item_deleter(world* const w) noexcept : world_ {w} { }

    void operator()(item_instance_id const id) const noexcept;
    world const& source_world() const noexcept { return *world_; }
private:
    world* world_;
};

using unique_item = std::unique_ptr<item_instance_id, item_deleter const&>;

} //namespace boken
