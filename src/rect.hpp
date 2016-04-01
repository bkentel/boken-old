#pragma once

#include <bkassert/assert.hpp>

#include "math_types.hpp"
#include "random.hpp"

namespace boken {

//! @returns true if the rectangle @p r can be split horizontally into two new
//! rectangles with a height at least @p min_h. Otherwise, false.
template <typename T, typename U>
constexpr bool can_slice_rect_x(
    axis_aligned_rect<T> const r, size_type_y<U> const min_h
) noexcept {
    return r.height() >= 2 * min_h;
}

//! @returns true if the rectangle @p r can be split vertically into two new
//! rectangles with a width at least @p min_w. Otherwise, false.
template <typename T, typename U>
constexpr bool can_slice_rect_y(
    axis_aligned_rect<T> const r, size_type_x<U> const min_w
) noexcept {
    return r.width() >= 2 * min_w;
}

//! @returns true if the rectangle @p r has a height exceeding @p max_h.
//! Otherwise, false.
template <typename T, typename U>
constexpr bool must_slice_rect_x(
    axis_aligned_rect<T> const r, size_type_y<U> const max_h
) noexcept {
    return r.height() > max_h;
}

//! @returns true if the rectangle @p r has a width exceeding @p max_w.
//! Otherwise, false.
template <typename T, typename U>
constexpr bool must_slice_rect_y(
    axis_aligned_rect<T> const r, size_type_x<U> const max_w
) noexcept {
    return r.width() > max_w;
}

enum rect_slice_result {
    none //!< no slice
  , x    //!< slice along the x axis
  , y    //!< slice along the y axis
  , xy   //!< slice along either / both the x and y axis
};

//! @returns what type or types of slicing are possible for the rectangle @p r
//! given the constraint that the resulting split rectangles can be no smaller
//! than @p min_w by @p min_h in size.
template <typename T, typename U, typename V>
constexpr rect_slice_result can_slice_rect(
    axis_aligned_rect<T> const r
  , size_type_x<U>       const min_w
  , size_type_y<V>       const min_h
) noexcept {
    return (can_slice_rect_x(r, min_h) && can_slice_rect_y(r, min_w))
             ? rect_slice_result::xy
         : can_slice_rect_x(r, min_h)
             ? rect_slice_result::x
         : can_slice_rect_y(r, min_w)
             ? rect_slice_result::y
             : rect_slice_result::none;
}

//! @returns what type or types of slicing are necessary for the rectangle @p r
//! given the constraint that @p r must be no bigger than @p max_w by @p max_h.
template <typename T, typename U, typename V>
constexpr rect_slice_result must_slice_rect(
    axis_aligned_rect<T> const r
  , size_type_x<U>       const max_w
  , size_type_y<V>       const max_h
) noexcept {
    return (must_slice_rect_x(r, max_h) && must_slice_rect_y(r, max_w))
             ? rect_slice_result::xy
         : must_slice_rect_x(r, max_h)
             ? rect_slice_result::x
         : must_slice_rect_y(r, max_w)
             ? rect_slice_result::y
             : rect_slice_result::none;
}

//! @returns true if @p a is larger than @p b, false if @p b is larger than @p a
//! and a random choice otherwise.
template <typename T>
bool choose_largest(random_state& rng, T const& a, T const& b) noexcept {
    return (a < b) ? false
         : (b < a) ? true
         : random_coin_flip(rng);
}

namespace detail {

template <typename T>
T random_bounded_normal_impl(
    std::integral_constant<int, 0> // integers
  , double const n
  , T const lo
  , T const hi
) noexcept {
    return clamp(round_as<T>(n), lo, hi);
}

template <typename T>
T random_bounded_normal_impl(
    std::integral_constant<int, 1> // floating point
  , double const n
  , T const lo
  , T const hi
) noexcept {
    return clamp(n, lo, hi);
}

} //namespace detail

//! @returns a randomly generated number on a normal distribution clamped to the
//! range given by @p lo and @p hi.
template <typename T>
T random_bounded_normal(
    random_state& rng
  , double const mean
  , double const variance
  , T const lo
  , T const hi
) noexcept {
    using type = std::integral_constant<int
        , std::is_integral<T>::value       ? 0
        : std::is_floating_point<T>::value ? 1
                                           : 2>;

    return detail::random_bounded_normal_impl(
        type {}, random_normal(rng, mean, variance), lo, hi);
}

//! @return The rectangle @p r sliced into two smaller rectangles along its
//! largest axis, or a random axis if square. Otherwise, if the constraints
//! @p min_w and @p min_h can't be satisfied, the original rectangle @p r.
template <typename T, typename R = axis_aligned_rect<T>>
std::pair<R, R> slice_rect(
    random_state&              rng
  , axis_aligned_rect<T> const r
  , size_type_x<T>       const min_w
  , size_type_y<T>       const min_h
  , double               const inv_variance = 4.0
) noexcept {
    auto const choose_split_offset = [&](auto const size, auto const min_size) noexcept {
        auto const s     = value_cast(size);
        auto const min_s = value_cast(min_size);

        return random_bounded_normal(rng
          , s / 2.0
          , s / inv_variance
          , min_s
          , s - min_s);
    };

    R r0 = r;
    R r1 = r;

    auto const split_x = [&]() noexcept {
        auto const n = choose_split_offset(r.height(), min_h);
        r1.y0 = (r0.y1 = r0.y0 + size_type_y<T> {n});
    };

    auto const split_y = [&]() noexcept {
        auto const n = choose_split_offset(r.width(), min_w);
        r1.x0 = (r0.x1 = r0.x0 + size_type_x<T> {n});
    };

    switch (can_slice_rect(r, min_w, min_h)) {
    default                      :            break;
    case rect_slice_result::none :            break;
    case rect_slice_result::x    : split_x(); break;
    case rect_slice_result::y    : split_y(); break;
    case rect_slice_result::xy :
        choose_largest(rng, value_cast(r.width()), value_cast(r.height()))
          ? split_y()
          : split_x();
        break;
    }

    return {r0, r1};
}

//! @returns a random point inside the rectangle given by @p r
template <typename T>
point2<T> random_point_in_rect(
    random_state& rng
  , axis_aligned_rect<T> const r
) noexcept {
    return {static_cast<T>(random_uniform_int(
                rng, value_cast(r.x0), value_cast(r.x1) - 1))
          , static_cast<T>(random_uniform_int(
                rng, value_cast(r.y0), value_cast(r.y1) - 1))
    };
}

//! @return a random rectangle that is strictly contained within @p r that meets
//! the size constraints given by @p min_w, @p max_w and @p min_h, @p max_h.
template <typename T>
axis_aligned_rect<T> random_sub_rect(
    random_state&              rng
  , axis_aligned_rect<T> const r
  , size_type_x<T>       const min_w
  , size_type_x<T>       const max_w
  , size_type_y<T>       const min_h
  , size_type_y<T>       const max_h
  , double               const inverse_variance = 6.0
) noexcept {
    BK_ASSERT(value_cast(min_w) >= 0 && min_w <= max_w
           && value_cast(min_h) >= 0 && min_h <= max_h
           && inverse_variance > 0.0);

    auto const new_size = [&](auto const size, auto const min, auto const max) noexcept {
        auto const lo    = min;
        auto const hi    = std::min(max, size);
        auto const range = value_cast(hi - lo);

        return (range == 0)
          ? value_cast(size)
          : random_bounded_normal(rng
                                , value_cast(lo) + range / 2.0
                                , range / inverse_variance
                                , value_cast(lo)
                                , value_cast(hi));
    };

    auto const new_offset = [&](auto const size) noexcept {
        return (size <= 0)
          ? T {0}
          : static_cast<T>(random_uniform_int(rng, 0, size));
    };

    auto const w = r.width();
    auto const h = r.height();

    T const new_w = new_size(w, min_w, max_w);
    T const new_h = new_size(h, min_h, max_h);
    T const new_x = new_offset(value_cast(w) - new_w);
    T const new_y = new_offset(value_cast(h) - new_h);

    return {r.top_left() + vec2<T> {new_x, new_y}
          , size_type_x<T> {new_w}
          , size_type_y<T> {new_h}};
}

} //namespace boken
