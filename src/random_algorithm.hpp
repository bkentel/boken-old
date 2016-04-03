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

    std::array<point2i32, buffer_size> points;

    for (T d = 0; d <= max_distance; ++d) {
        size_t last_index = 0;

        points_around(origin, d
          , [&](point2i32 const p) noexcept { points[last_index++] = p; });

        auto const first = begin(points);
        auto const last  = first + static_cast<ptrdiff_t>(last_index);

        std::shuffle(first, last, rng);

        auto const it = std::find_if(first, last, pred);
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

    std::array<point2i32, buffer_size> points;

    size_t i = 0;
    for_each_xy(r, [&](point2i32 const p) noexcept {
        points[i++] = p;
    });

    auto const first = begin(points);
    auto const last  = first + static_cast<ptrdiff_t>(i);

    std::shuffle(first, last, rng);
    std::for_each(first, last, f);
}

} //namespace boken
