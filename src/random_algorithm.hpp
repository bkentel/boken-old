#pragma once

#include "random.hpp"
#include "math.hpp"
#include "rect.hpp"

#include "bkassert/assert.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include <cstddef>

namespace boken {

template <typename T, typename FwdIt>
FwdIt fill_with_points_in(axis_aligned_rect<T> const r, FwdIt const first, FwdIt const last) {
    auto it = first;

    for_each_xy(r, [&](point2i32 const p) noexcept {
        if (it != last) {
            *it++ = p;
        }
    });

    return it;
}

template <typename T, typename FwdIt>
FwdIt fill_with_points_around(point2<T> const p, T const d, FwdIt const first, FwdIt const last) {
    auto it = first;

    points_around(p, d, [&](point2<T> const p) noexcept {
        if (it != last) {
            *it++ = p;
        }
    });

    return it;
}

template <typename T, typename Predicate>
std::pair<point2<T>, bool>
find_random_nearest(
    random_state&       rng
  , point2<T>     const origin
  , T             const max_distance
  , Predicate           pred
) {
    constexpr size_t buffer_size = 128;

    BK_ASSERT(max_distance >= 0
           && static_cast<size_t>(max_distance) <= buffer_size / 8);

    std::array<point2<T>, buffer_size> points;

    auto const p_first = begin(points);
    auto const p_last  = end(points);

    for (T d = 0; d <= max_distance; ++d) {
        auto const last = fill_with_points_around(origin, d, p_first, p_last);

        std::shuffle(p_first, last, rng);
        auto const it = std::find_if(p_first, last, pred);

        if (it != last) {
            return {*it, true};
        }
    }

    return {origin, false};
}

template <typename T, typename UnaryF>
void for_each_xy_random(random_state& rng, axis_aligned_rect<T> const r, UnaryF f) {
    constexpr size_t buffer_size = 128;

    {
        auto const area = value_cast(r.area());
        BK_ASSERT(area >= 0 && static_cast<size_t>(area) <= 128);
    }

    std::array<point2<T>, buffer_size> points;

    auto const first = begin(points);
    auto const last  = end(points);
    auto const it    = fill_with_points_in(r, first, last);

    std::shuffle(first, it, rng);
    std::for_each(first, it, f);
}

template <typename T, typename Predicate>
std::pair<point2<T>, bool>
find_if_random(random_state& rng, axis_aligned_rect<T> const r, Predicate pred) {
    constexpr size_t buffer_size = 128;

    {
        auto const area = value_cast(r.area());
        BK_ASSERT(area >= 0 && static_cast<size_t>(area) <= 512);
    }

    std::array<point2<T>, buffer_size> points;

    auto const p_first = begin(points);
    auto const p_last  = end(points);
    auto const last    = fill_with_points_in(r, p_first, p_last);

    std::shuffle(p_first, last, rng);

    auto const it = std::find_if(p_first, last, [&](auto const p) noexcept {
        return pred(p);
    });

    return (it == last)
      ? std::make_pair(r.top_left(), false)
      : std::make_pair(*it, true);
}

namespace detail {

template <size_t N, typename T, typename Check, typename Predicate>
uint32_t fold_neighbors_impl(
    std::array<int, N> const& xi
  , std::array<int, N> const& yi
  , point2<T> const p
  , Check check
  , Predicate pred
) noexcept {
    static_assert(noexcept(check(p)), "");
    static_assert(noexcept(pred(p)), "");
    static_assert(N <= 32, "");

    uint32_t result {};

    T const x = value_cast(p.x);
    T const y = value_cast(p.y);

    for (size_t i = 0; i < N; ++i) {
        auto const q = point2<T> {static_cast<T>(x + xi[i])
                                , static_cast<T>(y + yi[i])};

        uint32_t const bit = (check(q) && pred(q)) ? 1u : 0u;
        result |= bit << (N - 1u - i);
    }

    return result;
}

} //namespace detail

//     N[3]
// W[2]    E[1]
//     S[0]
template <typename T, typename Check, typename Predicate>
uint32_t fold_neighbors4(point2<T> const p, Check check, Predicate pred) noexcept {
    constexpr std::array<int, 4> yi {-1,  0, 0, 1};
    constexpr std::array<int, 4> xi { 0, -1, 1, 0};
    return detail::fold_neighbors_impl(xi, yi, p, check, pred);
}

// NW[7] N[6] NE[5]
//  W[4]       E[3]
// SW[2] S[1] SE[0]
template <typename T, typename Check, typename Predicate>
uint32_t fold_neighbors8(point2<T> const p, Check check, Predicate pred) noexcept {
    constexpr std::array<int, 8> yi {-1, -1, -1,  0, 0,  1, 1, 1};
    constexpr std::array<int, 8> xi {-1,  0,  1, -1, 1, -1, 0, 1};
    return detail::fold_neighbors_impl(xi, yi, p, check, pred);
}

} //namespace boken
