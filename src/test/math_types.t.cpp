#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "math_types.hpp"

namespace {

template <typename Test, typename T>
constexpr Test check_value_type(T const n) noexcept {
    using namespace boken;
    static_assert(std::is_same<Test, decltype(value_cast(n))>::value, "");
    return value_cast(n);
}

template <typename Test, typename T>
constexpr std::pair<Test, Test> check_value_type_xy(T const p) noexcept {
    return std::make_pair(check_value_type<Test>(p.x)
                        , check_value_type<Test>(p.y));
}

template <typename Test, typename T>
constexpr auto check_result_type(T const n, std::false_type) noexcept {
    static_assert(std::is_same<Test, typename T::type>::value, "");
    return n;
}

template <typename Test, typename T>
constexpr auto check_result_type(T const n, std::true_type) noexcept {
    static_assert(std::is_same<Test, T>::value, "");
    return n;
}

template <typename Test, typename T>
constexpr auto check_result_type(T const n) noexcept {
    return check_result_type<Test>(n, std::is_arithmetic<T> {});
}

} //namespace

TEST_CASE("basic_1_tuple", "[math]") {
    using namespace boken;

    constexpr auto P = int16_t {1};
    constexpr auto Q = int32_t {2};
    constexpr auto U = int16_t {3};
    constexpr auto V = int32_t {4};

    offi16x  const p = P;
    offi32x  const q = Q;
    sizei16x const u = U;
    sizei32x const v = V;

    // initial value
    {
        REQUIRE(check_result_type<int16_t>(value_cast(p)) == P);
        REQUIRE(check_result_type<int32_t>(value_cast(q)) == Q);
        REQUIRE(check_result_type<int16_t>(value_cast(u)) == U);
        REQUIRE(check_result_type<int32_t>(value_cast(v)) == V);
    }

    // construction
    {
        offi16x  const p0 = p; // from same type
        offi32x  const q0 = p; // from smaller type
        sizei16x const u0 = u; // from same type
        sizei32x const v0 = u; // from smaller type

        REQUIRE(check_result_type<int16_t>(value_cast(p0)) == P);
        REQUIRE(check_result_type<int32_t>(value_cast(q0)) == P);
        REQUIRE(check_result_type<int16_t>(value_cast(u0)) == U);
        REQUIRE(check_result_type<int32_t>(value_cast(v0)) == U);
    }

    // self comparison
    {
        REQUIRE(p == p);
        REQUIRE(q == q);
        REQUIRE(u == u);
        REQUIRE(v == v);

        REQUIRE_FALSE(p != p);
        REQUIRE_FALSE(q != q);
        REQUIRE_FALSE(u != u);
        REQUIRE_FALSE(v != v);

        REQUIRE_FALSE(p < p);
        REQUIRE_FALSE(q < q);
        REQUIRE_FALSE(u < u);
        REQUIRE_FALSE(v < v);

        REQUIRE(p <= p);
        REQUIRE(q <= q);
        REQUIRE(u <= u);
        REQUIRE(v <= v);

        REQUIRE_FALSE(p > p);
        REQUIRE_FALSE(q > q);
        REQUIRE_FALSE(u > u);
        REQUIRE_FALSE(v > v);

        REQUIRE(p >= p);
        REQUIRE(q >= q);
        REQUIRE(u >= u);
        REQUIRE(v >= v);
    }

    // comparison with a smaller type
    {
        REQUIRE(p != q);
        REQUIRE(q != p);
        REQUIRE(p <  q);
        REQUIRE(p <= q);
        REQUIRE(q >  p);
        REQUIRE(q >= p);

        REQUIRE(u != v);
        REQUIRE(v != u);
        REQUIRE(u <  v);
        REQUIRE(u <= v);
        REQUIRE(v >  u);
        REQUIRE(v >= u);
    }

    // arithmetic
    {
        // size + size = size
        REQUIRE(value_cast(check_result_type<int16_t>(u + u)) == (U + U));
        REQUIRE(value_cast(check_result_type<int32_t>(u + v)) == (U + V));
        REQUIRE(value_cast(check_result_type<int32_t>(v + u)) == (V + U));
        REQUIRE(value_cast(check_result_type<int32_t>(v + v)) == (V + V));

        // size - size = size
        REQUIRE(value_cast(check_result_type<int16_t>(u - u)) == (U - U));
        REQUIRE(value_cast(check_result_type<int32_t>(u - v)) == (U - V));
        REQUIRE(value_cast(check_result_type<int32_t>(v - u)) == (V - U));
        REQUIRE(value_cast(check_result_type<int32_t>(v - v)) == (V - V));
    }
}

TEST_CASE("basic_2_tuple", "[math]") {
    using namespace boken;

    constexpr auto PX = int16_t {1};
    constexpr auto PY = int16_t {2};
    constexpr auto QX = int32_t {0};
    constexpr auto QY = int32_t {1};

    constexpr auto p = point2i16 {PX, PY};
    constexpr auto q = point2i32 {QX, QY};
    constexpr auto u = vec2i16   {PX, PY};
    constexpr auto v = vec2i32   {QX, QY};

    // initial value
    {
        REQUIRE(check_result_type<int16_t>(value_cast(p.x)) == PX);
        REQUIRE(check_result_type<int16_t>(value_cast(p.y)) == PY);

        REQUIRE(check_result_type<int32_t>(value_cast(q.x)) == QX);
        REQUIRE(check_result_type<int32_t>(value_cast(q.y)) == QY);

        REQUIRE(check_result_type<int16_t>(value_cast(u.x)) == PX);
        REQUIRE(check_result_type<int16_t>(value_cast(u.y)) == PY);

        REQUIRE(check_result_type<int32_t>(value_cast(v.x)) == QX);
        REQUIRE(check_result_type<int32_t>(value_cast(v.y)) == QY);
    }

    auto const check = [](auto const a, auto const op, auto const b, auto const x, auto const y) noexcept {
        auto const c = op(a, b);

        REQUIRE(c.x == op(a.x, b.x));
        REQUIRE(c.y == op(a.y, b.y));

        REQUIRE(value_cast(c.x) == x);
        REQUIRE(value_cast(c.y) == y);

        static_assert(std::is_same<typename decltype(c)::type
                                 , std::decay_t<decltype(x)>> {}, "");
        static_assert(std::is_same<typename decltype(c)::type
                                 , std::decay_t<decltype(y)>> {}, "");

        return c;
    };

    // arithmetic +
    {
        auto const op = std::plus<> {};

        check(u, op, u, static_cast<int16_t>(PX + PX), static_cast<int16_t>(PY + PY));
        check(u, op, v, static_cast<int32_t>(PX + QX), static_cast<int32_t>(PY + QY));
        check(v, op, u, static_cast<int32_t>(PX + QX), static_cast<int32_t>(PY + QY));
        check(v, op, v, static_cast<int32_t>(QX + QX), static_cast<int32_t>(QY + QY));

        check(p, op, u, static_cast<int16_t>(PX + PX), static_cast<int16_t>(PY + PY));
        check(p, op, v, static_cast<int32_t>(PX + QX), static_cast<int32_t>(PY + QY));
        check(q, op, u, static_cast<int32_t>(PX + QX), static_cast<int32_t>(PY + QY));
        check(q, op, v, static_cast<int32_t>(QX + QX), static_cast<int32_t>(QY + QY));
    }

    // arithmetic -
    {
        auto const op = std::minus<> {};

        check(u, op, u, int16_t {0}, int16_t {0});
        check(u, op, v, static_cast<int32_t>(PX - QX), static_cast<int32_t>(PY - QY));
        check(v, op, u, static_cast<int32_t>(QX - PX), static_cast<int32_t>(QY - PY));
        check(v, op, v, int32_t {0}, int32_t {0});

        check(p, op, u, int16_t {0}, int16_t {0});
        check(p, op, v, static_cast<int32_t>(PX - QX), static_cast<int32_t>(PY - QY));
        check(q, op, u, static_cast<int32_t>(QX - PX), static_cast<int32_t>(QY - PY));
        check(q, op, v, int32_t {0}, int32_t {0});
    }

    // arithmetic *
    {
        REQUIRE((u * 2) == (2 * u));
        REQUIRE((v * 2) == (2 * v));
        REQUIRE((p * 2) == (2 * p));
        REQUIRE((q * 2) == (2 * q));

        REQUIRE((u * 2).x == (u.x * 2));
        REQUIRE((u * 2).y == (u.y * 2));
        REQUIRE((p * 2).x == (p.x * 2));
        REQUIRE((p * 2).y == (p.y * 2));

        REQUIRE(value_cast((u * 2).x) == (PX * 2));
        REQUIRE(value_cast((u * 2).y) == (PY * 2));
        REQUIRE(value_cast((p * 2).x) == (PX * 2));
        REQUIRE(value_cast((p * 2).y) == (PY * 2));
    }

    // arithmetic /
    {
        REQUIRE((u / 2) == (2 / u));
        REQUIRE((v / 2) == (2 / v));
        REQUIRE((p / 2) == (2 / p));
        REQUIRE((q / 2) == (2 / q));

        REQUIRE((u / 2).x == (u.x / 2));
        REQUIRE((u / 2).y == (u.y / 2));
        REQUIRE((p / 2).x == (p.x / 2));
        REQUIRE((p / 2).y == (p.y / 2));

        REQUIRE(value_cast((u / 2).x) == (PX / 2));
        REQUIRE(value_cast((u / 2).y) == (PY / 2));
        REQUIRE(value_cast((p / 2).x) == (PX / 2));
        REQUIRE(value_cast((p / 2).y) == (PY / 2));
    }
}

TEST_CASE("axis_aligned_rect", "[math]") {
    using namespace boken;

    auto const r0 = recti32 {
        offset_type_x<uint8_t>  {uint8_t   {0}}
      , offset_type_y<uint16_t> {uint16_t  {0}}
      , offset_type_x<int16_t>  {int16_t  {10}}
      , offset_type_y<int32_t>  {int32_t  {10}}
    };

    auto const r1 = recti32 {
        offset_type_x<uint8_t>  {uint8_t   {0}}
      , offset_type_y<uint16_t> {uint16_t  {0}}
      , size_type_x<int16_t>    {int16_t  {10}}
      , size_type_y<int32_t>    {int32_t  {10}}
    };

    auto const r2 = recti32 {
        point2i16            {uint8_t {0}, uint8_t {0}}
      , size_type_x<int16_t> {int16_t  {10}}
      , size_type_y<int32_t> {int32_t  {10}}
    };

    auto const r3 = recti32 {
        point2i16 {uint8_t {0},  uint8_t {0}}
      , point2i16 {uint8_t {10}, uint8_t {10}}
    };

    REQUIRE(r0 == r0);
    REQUIRE(r0 == r1);
    REQUIRE(r1 == r0);
    REQUIRE(r1 == r2);
    REQUIRE(r2 == r1);
    REQUIRE(r2 == r3);
    REQUIRE(r3 == r2);
    REQUIRE(r3 == r3);
}

#endif // !defined(BK_NO_TESTS)
