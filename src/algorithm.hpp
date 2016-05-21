#pragma once

#include "math_types.hpp"

#include <algorithm>
#include <numeric>
#include <type_traits>
#include <memory>

#include <cstddef>

namespace boken {

template <typename T, T Value>
struct always_same {
    template <typename... Args>
    inline constexpr T operator()(Args&&...) const noexcept {
        return Value;
    }
};

using always_true  = always_same<bool, true>;
using always_false = always_same<bool, false>;

struct ignore {
    template <typename... Args>
    inline constexpr void operator()(Args&&...) const noexcept {
    }
};

//! Return an iterator to the @p n th value equal to @p value in the range given
//! by [@p first, @p last). Otherwise, return @p last.
template <typename FwdIt, typename Count, typename Value>
auto find_nth(FwdIt const first, FwdIt const last, Count const n, Value const& value) noexcept {
    static_assert(noexcept(value == value), "");
    static_assert(std::is_integral<Count>::value, "");

    return std::find_if(first, last
      , [&value, n, i = Count {0}](auto const& v) mutable noexcept {
            return (v == value) && (++i > n);
        });
}

//! Container wide version of find_nth.
template <typename Container, typename Count, typename Value>
auto find_nth(Container&& c, Count const n, Value const& value) noexcept {
    using std::begin;
    using std::end;
    return find_nth(begin(c), end(c), n, value);
}

//! Container wide version of std::fill.
template <typename Container, typename Value>
void fill(Container& c, Value const& v) {
    using std::begin;
    using std::end;

    std::fill(begin(c), end(c), v);
}

//! Write to @p out increasing values starting at @p i for each element in the
//! range given by [first, last) that @p pred returns true for.
template <typename FwdIt, typename Index, typename Out, typename Predicate>
void fill_with_index_if(FwdIt first, FwdIt last, Index i, Out out, Predicate pred) {
    for (auto it = first; it != last; ++it) {
        if (pred(*it)) {
            *out = i++;
            ++out;
        }
    }
}

template <typename Container, typename Index, typename Out, typename Predicate>
void fill_with_index_if(Container&& c, Index i, Out out, Predicate pred) {
    fill_with_index_if(begin(c), end(c), i , out, pred);
}

template <typename InputIt, typename I, typename OutIt, typename Predicate>
void copy_index_if(
    InputIt const first
  , InputIt const last
  , I i
  , OutIt out
  , Predicate pred
) {
    for (auto it = first; it != last; ++it, ++i) {
        if (pred(*it)) {
            *out = i;
            ++out;
        }
    }
}

template <typename Container, typename I, typename OutIt, typename Predicate>
void copy_index_if(Container&& c, I i, OutIt out, Predicate pred) {
    copy_index_if(begin(c), end(c), i, out, pred);
}


//! Invoke f for each index i in the range [first, last) with the ith element
//! of c. For values of i which are out of range, do nothing.
template <typename Container, typename FwdIt, typename UnaryF>
void for_each_index_of(Container&& c, FwdIt first, FwdIt last, UnaryF f) {
    using index_t = std::decay_t<decltype(*first)>;
    static_assert(std::is_arithmetic<index_t> {}, "");

    if (first == last) {
        return;
    }

    auto i = index_t {};
    auto j = *first;

    for (auto&& e : c) {
        if (i++ != j) {
            continue;
        }

        f(e);

        if (++first == last) {
            return;
        }

        j = *first;
    }
}

//! A 2d-array adapter for a linear array.
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

template<typename AssociativeContainer
       , typename Key = typename AssociativeContainer::key_type>
auto find_or_nullptr(AssociativeContainer&& c, Key const& key) noexcept {
    using std::end;

    auto const it = c.find(key);
    return (it == end(c))
      ? nullptr
      : std::addressof(it->second);
}

template <typename FwdIt, typename Predicate, typename UnaryF>
void for_each_matching(
    FwdIt const first
  , FwdIt const last
  , Predicate   pred
  , UnaryF      callback
) {
    std::for_each(first, last, [&](auto const& element) {
        if (pred(element)) {
            callback(element);
        }
    });
}

template <typename Container, typename Predicate, typename UnaryF>
void for_each_matching(
    Container&& c
  , Predicate   pred
  , UnaryF      callback
) {
    using std::begin;
    using std::end;

    for_each_matching(begin(c), end(c), pred, callback);
}

template <typename T, typename U>
constexpr int compare(T const& lhs, U const& rhs) noexcept {
    return lhs <  rhs ? -1
         : lhs == rhs ? 0
                      : 1;
}

} //namespace boken
