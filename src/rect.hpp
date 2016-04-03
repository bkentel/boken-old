#pragma once

#include <bkassert/assert.hpp>

#include "math.hpp"
#include "random.hpp"

namespace boken {

//! Type trait for the number of parameters a function (object) takes.
template <typename F>
struct arity_of;

template <typename R, typename... Args>
struct arity_of<R (*)(Args...)> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct arity_of<R (C::*)(Args...)> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct arity_of<R (C::*)(Args...) const> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename R, typename... Args>
struct arity_of<R (Args...)> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename F>
struct arity_of {
    static_assert(std::is_class<std::decay_t<F>>::value, "");
    static constexpr size_t value = arity_of<decltype(&F::operator())>::value;
};

//! @return a rectangle that has been shrunk symmetrically by @p size on all
//! sides.
template <typename T, typename U = T>
axis_aligned_rect<T> shrink_rect(
    axis_aligned_rect<T> const r
  , U                    const size = U {1}
) noexcept {
    BK_ASSERT(size >= 0
           && size <= value_cast(r.width())
           && size <= value_cast(r.height()));

    auto const v = vec2<U> {size, size};
    return {r.top_left() + v, r.bottom_right() - v};
}

//! @return a rectangle that has been expanded symmetrically by @p size on all
//! sides.
template <typename T, typename U = T>
axis_aligned_rect<T> grow_rect(
    axis_aligned_rect<T> const r
  , U                    const size = U {1}
) noexcept {
    BK_ASSERT(size >= 0
           && size <= value_cast(r.width())
           && size <= value_cast(r.height()));

    auto const v = vec2<U> {size, size};
    return {r.top_left() - v, r.bottom_right() + v};
}

template <typename T> inline constexpr
axis_aligned_rect<T> move_to_origin(axis_aligned_rect<T> const r) noexcept {
    return r + (point2<T> {0, 0} - r.top_left());
}

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

namespace detail {

template <typename T, typename BinaryF>
void for_each_xy_impl(
    std::integral_constant<int, 2>
  , axis_aligned_rect<T> const r
  , BinaryF f
) {
    auto const x0 = value_cast(r.x0);
    auto const x1 = value_cast(r.x1);
    auto const y0 = value_cast(r.y0);
    auto const y1 = value_cast(r.y1);

    if (x1 - x0 <= T {0} || y1 - y0 <= T {0}) {
        return;
    }

    for (auto y = y0; y < y1; ++y) {
        bool const on_edge_y = (y == y0) || (y == y1 - 1);
        for (auto x = x0; x < x1; ++x) {
            bool const on_edge = on_edge_y || (x == x0) || (x == x1 - 1);
            f(point2<T> {x, y}, on_edge);
        }
    }
}

template <typename T, typename UnaryF>
void for_each_xy_impl(
    std::integral_constant<int, 1>
  , axis_aligned_rect<T> const r
  , UnaryF f
) {
    auto const x0 = value_cast(r.x0);
    auto const x1 = value_cast(r.x1);
    auto const y0 = value_cast(r.y0);
    auto const y1 = value_cast(r.y1);

    if (x1 - x0 <= T {0} || y1 - y0 <= T {0}) {
        return;
    }

    for (auto y = y0; y < y1; ++y) {
        for (auto x = x0; x < x1; ++x) {
            f(point2<T> {x, y});
        }
    }
}

} //namespace detail

//! Invoke the function @p f for every point in the rectangle @p r.
//! @param f can be of the form:
//! (1) void f(point2<T>) or
//! (2) void f(point2<T>, bool)
//! For (2), the bool parameter indicated whether the first parameter is on the
//! the perimeter of @p r.
template <typename T, typename F>
void for_each_xy(axis_aligned_rect<T> const r, F f) {
    constexpr int n = arity_of<F>::value;
    detail::for_each_xy_impl(std::integral_constant<int, n> {}, r, f);
}

//! Invoke the function @p f for every point on the perimeter of the rectangle
//! @p r.
//! @param f A unary function of the form void f(point2<T>).
template <typename T, typename UnaryF>
void for_each_xy_edge(axis_aligned_rect<T> const r, UnaryF f) {
    auto const x0 = value_cast(r.x0);
    auto const x1 = value_cast(r.x1);
    auto const y0 = value_cast(r.y0);
    auto const y1 = value_cast(r.y1);

    auto const w = value_cast(r.width());
    auto const h = value_cast(r.height());

    if (w <= T {0} || h <= T {0}) {
        return;
    }

    auto const type = ((w > T {1}) ? 1 : 0) | ((h > T {1}) ? 2 : 0);

    switch (type) {
    case 0: // w <= 1 && h <= 1
        f (r.top_left());
        break;
    case 1: // w > 1  && h == 1
        // top edge
        for (auto x = x0; x < x1; ++x) {
            f(point2<T> {x, y0});
        }
        break;
    case 2: // w == 1 && h > 1
        // left edge
        for (auto y = y0; y < y1; ++y) {
            f(point2<T> {x0, y});
        }
        break;
    case 3: // w > 1  && h > 1
        // top edge
        for (auto x = x0; x < x1; ++x) {
            f(point2<T> {x, y0});
        }

        // left and right edge
        for (auto y = y0 + 1; y < y1 - 1; ++y) {
            auto const x = static_cast<T>(x1 - 1);
            f(point2<T> {x0, y});
            f(point2<T> {x,  y});
        }

        // bottom edge
        for (auto x = x0; x < x1; ++x) {
            auto const y = static_cast<T>(y1 - 1);
            f(point2<T> {x, y});
        }
        break;
    default:
        break;
    }
}

//! Invoke @p center and @p edge as in for_each_xy and for_each_xy_edge
//! respectfully.
//!
//! 1111111111
//! 2000000002
//! 2000000002
//! 2000000002
//! 3333333333
//!
template <typename T, typename CenterF, typename EdgeF>
void for_each_xy_center_first(axis_aligned_rect<T> const r, CenterF center, EdgeF edge) {
    for_each_xy(shrink_rect(r), center);
    for_each_xy_edge(r, edge);
}

//! Invoke the function @p f for every point @p distance from the point @p p.
//! @param f A unary function of the form void f(point2<T>).
template <typename T, typename UnaryF>
void points_around(point2<T> const p, T const distance, UnaryF f) {
    auto const s = static_cast<T>(distance * T {2} + T {1});
    auto const r = axis_aligned_rect<T> {
        p - vec2<T> {distance, distance}
      , size_type_x<T> {s}
      , size_type_y<T> {s}};

    for_each_xy_edge(r, f);
}

template <typename T, typename Predicate>
std::pair<point2<T>, bool>
find_if(axis_aligned_rect<T> const r, Predicate pred) {
    auto const x0 = value_cast(r.x0);
    auto const x1 = value_cast(r.x1);
    auto const y0 = value_cast(r.y0);
    auto const y1 = value_cast(r.y1);

    if (x1 - x0 <= T {0} || y1 - y0 <= T {0}) {
        return {r.top_left(), false};
    }

    for (auto y = y0; y < y1; ++y) {
        for (auto x = x0; x < x1; ++x) {
            auto const p = point2<T> {x, y};
            if (pred(p)) {
                return {p, true};
            }
        }
    }

    return {r.top_left(), false};
}

} //namespace boken
