#pragma once

#include <type_traits>

#include <cstddef>

namespace boken {

namespace detail {

//! convert a sequence of bit numbers to an unsigned integral type.
template <typename Tag>
struct bits_to_int {
    static constexpr size_t size = Tag::size;
    using result = typename Tag::type;

    template <size_t... Bits>
    static constexpr result value(std::integer_sequence<size_t, Bits...>) noexcept {
        return value<void, Bits...>(0);
    }

    template <typename, size_t Bit, size_t... Bits>
    static constexpr result value(int) noexcept {
        static_assert(Bit < size, "");
        return static_cast<result>((1u << Bit) | value<void, Bits...>(0));
    }

    template <typename>
    static constexpr result value(int) noexcept {
        return result {0};
    }
};

} // namespace detail

template <typename Tag, size_t... Bits>
struct flag_t {
    static constexpr size_t size = Tag::size;
    using type = typename Tag::type;

    static constexpr type value = detail::bits_to_int<Tag>::value(
        std::integer_sequence<size_t, Bits...> {});
};

template <typename Tag>
class flag_set {
    template <size_t... Values>
    using seq = std::integer_sequence<size_t, Values...>;
public:
    static constexpr size_t bits = Tag::size;
    using storage_type = typename Tag::type;

    static_assert(std::is_unsigned<storage_type>::value, "");
    static_assert(bits > 0u && bits <= sizeof(storage_type) * 8u, "");

    template <size_t... Bits>
    using flag_type = flag_t<Tag, Bits...>;

    constexpr flag_set() noexcept = default;
    constexpr explicit flag_set(storage_type const n) noexcept
      : data_ {n}
    {
    }

    template <size_t... Bits>
    constexpr flag_set(flag_type<Bits...>) noexcept
      : data_ {flag_type<Bits...>::value}
    {
    }

    constexpr bool none() const noexcept { return data_ == storage_type {0}; }
    constexpr bool any()  const noexcept { return data_ != storage_type {0}; }

    //! true if at least one bit in bits is set; false otherwise.
    template <size_t... Bits>
    constexpr bool any(flag_type<Bits...>) const noexcept {
        return any_(seq<Bits...> {});
    }

    //! true if one or more of the bits are set and no other bits are set;
    //! false otherwise.
    template <size_t... Bits>
    constexpr bool exclusive_any(flag_type<Bits...>) const noexcept {
        return any_(seq<Bits...> {})
            && !(data_ & (~flag_set {flag_type<Bits...> {}}).data_);
    }

    //! true if all of the bits are set; false otherwise.
    template <size_t... Bits>
    constexpr bool test(flag_type<Bits...>) const noexcept {
        return test_(seq<Bits...> {});
    }

    template <size_t... Bits>
    void set(flag_type<Bits...>) noexcept {
        return set_(seq<Bits...> {});
    }

    template <size_t... Bits>
    void clear(flag_type<Bits...>) noexcept {
        return clear_(seq<Bits...> {});
    }

    //! flip all used bits (the bits constant) leaving the unused bits 0.
    flag_set operator~() const noexcept {
        return flag_set {~data_ & ((1u << (bits)) - 1u)};
    }

    constexpr bool operator==(flag_set<Tag> const other) const noexcept {
        return data_ == other.data_;
    }

    template <size_t... Bits>
    constexpr bool operator==(flag_type<Bits...>) const noexcept {
        return data_ == flag_type<Bits...>::value;
    }
private:
    template <size_t Head, size_t... Tail>
    void set_(seq<Head, Tail...>) noexcept {
        static_assert(Head < bits, "");
        (data_ |= (1u << Head)), set_(seq<Tail...> {});
    }

    void set_(seq<>) noexcept { }

    template <size_t Head, size_t... Tail>
    void clear_(seq<Head, Tail...>) noexcept {
        static_assert(Head < bits, "");
        (data_ &= ~(1u << Head)), clear_(seq<Tail...> {});
    }

    void clear_(seq<>) noexcept { }

    template <size_t Head, size_t... Tail>
    constexpr bool test_(seq<Head, Tail...>) const noexcept {
        static_assert(Head < bits, "");
        return ((data_ & (1u << Head)) != 0u)
            && test_(seq<Tail...> {});
    }

    constexpr bool test_(seq<>) const noexcept { return true; }

    template <size_t Head, size_t... Tail>
    constexpr bool any_(seq<Head, Tail...>) const noexcept {
        static_assert(Head < bits, "");
        return ((data_ & (1u << Head)) != 0u)
            || any_(seq<Tail...> {});
    }

    constexpr bool any_(seq<>) const noexcept { return false; }

    storage_type data_ {};
};

template <typename Tag, size_t... Bits0, size_t... Bits1>
constexpr flag_set<Tag> operator|(flag_t<Tag, Bits0...>, flag_t<Tag, Bits1...>) noexcept {
    return flag_set<Tag> {
        flag_t<Tag, Bits0...>::value | flag_t<Tag, Bits1...>::value
    };
}

} // namespace boken
