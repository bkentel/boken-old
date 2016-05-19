#pragma once

#include "random.hpp"
#include "math.hpp"
#include "rect.hpp"

#include "bkassert/assert.hpp"

#include <algorithm>
#include <array>
#include <utility>
#include <iterator>

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

    points_around(p, d, [&](point2<T> const q) noexcept {
        if (it != last) {
            *it++ = q;
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

        shuffle(rng, p_first, last);
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

    shuffle(rng, first, it);
    std::for_each(first, it, f);
}

template <typename T, typename Predicate>
std::pair<point2<T>, bool>
find_if_random(random_state& rng, axis_aligned_rect<T> const r, Predicate pred) {
    constexpr size_t buffer_size = 512;

    {
        auto const area = value_cast(r.area());
        BK_ASSERT(area >= 0 && static_cast<size_t>(area) <= buffer_size);
    }

    std::array<point2<T>, buffer_size> points;

    auto const p_first = begin(points);
    auto const p_last  = end(points);
    auto const last    = fill_with_points_in(r, p_first, p_last);

    shuffle(rng, p_first, last);

    auto const it = std::find_if(p_first, last, [&](auto const p) noexcept {
        return pred(p);
    });

    return (it == last)
      ? std::make_pair(r.top_left(), false)
      : std::make_pair(*it, true);
}

template <typename T = int32_t>
vec2<T> random_dir4(random_state& rng) noexcept {
    static_assert(std::is_signed<T> {}, "");

    constexpr std::array<int, 4> dir_x {-1,  0, 0, 1};
    constexpr std::array<int, 4> dir_y { 0, -1, 1, 0};

    auto const i = static_cast<size_t>(random_uniform_int(rng, 0, 3));
    return {static_cast<T>(dir_x[i]), static_cast<T>(dir_y[i])};
}

template <typename T = int32_t>
vec2<T> random_dir8(random_state& rng) noexcept {
    static_assert(std::is_signed<T> {}, "");

    constexpr std::array<int, 8> dir_x {-1,  0,  1, -1, 1, -1, 0, 1};
    constexpr std::array<int, 8> dir_y {-1, -1, -1,  0, 0,  1, 1, 1};

    auto const i = static_cast<size_t>(random_uniform_int(rng, 0, 7));
    return {static_cast<T>(dir_x[i]), static_cast<T>(dir_y[i])};
}

template <typename RanIt>
inline void shuffle(random_state& rng, RanIt const first, RanIt const last) noexcept {
    RanIt next = first;
    for (int32_t i = 1; ++next != last; ++i) {
        auto const off = random_uniform_int(rng, 0, i);
        std::swap(*next, *(first + off));
    }
}

template <typename RndIt>
RndIt random_value_in_range(random_state& rng, RndIt const first, RndIt const last) noexcept {
    using Cat = typename std::iterator_traits<RndIt>::iterator_category;
    static_assert(std::is_same<Cat, std::random_access_iterator_tag> {}, "");

    auto const n = static_cast<int32_t>(std::distance(first, last));
    return (n >  1) ? (first + random_uniform_int(rng, 0, n - 1))
         : (n == 1) ? first
                    : last;
}


} //namespace boken
