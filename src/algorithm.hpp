#pragma once

#include "math_types.hpp"

#include <type_traits>
#include <cstddef>

namespace boken {

struct always_true {
    template <typename... Args>
    inline constexpr bool operator()(Args...) const noexcept {
        return true;
    }
};

struct always_false {
    template <typename... Args>
    inline constexpr bool operator()(Args...) const noexcept {
        return false;
    }
};

template <typename Container, typename T, typename U>
auto&& at_xy(Container&& c, T const x, T const y, U const w) noexcept {
    using type = typename std::decay_t<Container>::iterator::iterator_category;
    static_assert(std::is_same<type, std::random_access_iterator_tag> {}, "");
    static_assert(std::is_arithmetic<T> {}, "");
    static_assert(std::is_arithmetic<U> {}, "");

    return c[static_cast<size_t>(x)
           + static_cast<size_t>(y) * static_cast<size_t>(w)];
}

template <typename Container, typename T, typename U>
auto&& at_xy(Container&& c, point2<T> const p, size_type_x<U> const w) noexcept {
    return at_xy(std::forward<Container>(c)
               , value_cast(p.x), value_cast(p.y), value_cast(w));
}

namespace detail {

template <typename Container, typename T>
struct at_xy_getter {
    Container&& c_;
    T const     w_;

    template <typename U>
    auto&& operator()(U const x, U const y) const noexcept {
        return at_xy(c_, x, y, w_);
    }

    template <typename U>
    auto&& operator()(point2<U> const p) const noexcept {
        return (*this)(value_cast(p.x), value_cast(p.y));
    }
};

} // namespace detail

template <typename Container, typename T>
auto make_at_xy_getter(Container&& c, T const w) noexcept {
    static_assert(!std::is_rvalue_reference<Container> {}, "");

    auto const w_value = value_cast(w);
    return detail::at_xy_getter<Container, decltype(w_value)> {
        std::forward<Container>(c), w_value};
}

} //namespace boken
