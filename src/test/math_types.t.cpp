#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "math_types.hpp"
#include "math.hpp"

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

//TEST_CASE("basic_2_tuple", "[math]") {
//    using namespace boken;
//
//    SECTION("construction") {
//        point2i16 const p0 {int16_t {1}, int16_t {1}}; // from constant
//        point2i32 const q0 {int16_t {2}, int16_t {2}}; // from smaller constant type
//        point2i32 const p1 = p0;                       // from smaller type
//        point2i16 const q1 = p0;                       // from same size type
//
//        REQUIRE(check_value_type_xy<int16_t>(p0)
//             == std::make_pair(int16_t {1}, int16_t {1}));
//        REQUIRE(check_value_type_xy<int32_t>(q0) == std::make_pair(2, 2));
//        REQUIRE(check_value_type_xy<int32_t>(p1) == std::make_pair(1, 1));
//        REQUIRE(check_value_type_xy<int16_t>(q1)
//             == std::make_pair(int16_t {1}, int16_t {1}));
//    }
//
//    SECTION("comparison") {
//        point2i16 const p = {int16_t {1}, int16_t {1}};
//        point2i32 const q = {int32_t {2}, int16_t {1}};
//        point2i32 const r = {int32_t {1}, int16_t {1}};
//        vec2i16   const u = {int16_t {1}, int16_t {1}};
//        vec2i32   const v = {int32_t {2}, int16_t {1}};
//        vec2i32   const w = {int32_t {1}, int16_t {1}};
//
//        // self equality
//        REQUIRE(p == p);
//        REQUIRE(q == q);
//        REQUIRE(r == r);
//
//        REQUIRE(u == u);
//        REQUIRE(v == v);
//        REQUIRE(w == w);
//
//        // self unequality
//        REQUIRE_FALSE(p != p);
//        REQUIRE_FALSE(q != q);
//        REQUIRE_FALSE(r != r);
//
//        REQUIRE_FALSE(u != u);
//        REQUIRE_FALSE(v != v);
//        REQUIRE_FALSE(w != w);
//
//        // equality with diff. underlying types
//        REQUIRE(p == r);
//        REQUIRE(r == p);
//
//        REQUIRE(u == w);
//        REQUIRE(w == u);
//
//        // unequality with diff. underlying types
//        REQUIRE(p != r);
//        REQUIRE(r != p);
//
//        REQUIRE(u != w);
//        REQUIRE(w != u);
//    }
//
//    SECTION("arithmetic") {
//        point2i16 const p = {int16_t {1}, int16_t {1}};
//        point2i32 const q = {int32_t {2}, int16_t {1}};
//        vec2i16   const u = {int16_t {1}, int16_t {1}};
//        vec2i32   const v = {int32_t {2}, int16_t {1}};
//
//        // vector + vector
//        REQUIRE(check_value_type_xy<int32_t>(u + v)
//             == std::make_pair<int32_t>(3, 1));
//
//        // vector - vector
//        REQUIRE(check_value_type_xy<int32_t>(u - v)
//             == std::make_pair<int32_t>(-1, 0));
//
//        // point + vector
//        REQUIRE(check_value_type_xy<int32_t>(p + v)
//             == std::make_pair<int32_t>(3, 2));
//
//        // point - vector
//        REQUIRE(check_value_type_xy<int32_t>(q - u)
//             == std::make_pair<int32_t>(1, 0));
//
//        // point - point
//        REQUIRE(check_value_type_xy<int32_t>(p - q)
//             == std::make_pair<int32_t>(-1, 0));
//
//        // vector * constant
//        REQUIRE(check_value_type_xy<int32_t>(u * 1)
//             == std::make_pair<int32_t>(1, 1));
//
//        // point * constant
//        REQUIRE(check_value_type_xy<int32_t>(p * 1)
//             == std::make_pair<int32_t>(1, 1));
//
//        // vector / constant
//        REQUIRE(check_value_type_xy<int32_t>(u / 1)
//             == std::make_pair<int32_t>(1, 1));
//
//        // point / constant
//        REQUIRE(check_value_type_xy<int32_t>(p / 1)
//             == std::make_pair<int32_t>(1, 1));
//
//        // mutating assignment vector & vector
//        vec2i32 w = {1, 1};
//
//        w += v;
//        REQUIRE((w == vec2i32 {3, 2}));
//
//        w -= v;
//        REQUIRE((w == vec2i32 {1, 1}));
//
//        w *= 10;
//        REQUIRE((w == vec2i32 {10, 10}));
//
//        w /= 10;
//        REQUIRE((w == vec2i32 {1, 1}));
//
//        // mutating assignment point & vector
//        point2i32 r = {1, 1};
//
//        r += u;
//        REQUIRE((r == point2i32 {2, 2}));
//
//        r -= u;
//        REQUIRE((r == point2i32 {1, 1}));
//
//        r *= 10;
//        REQUIRE((r == point2i32 {10, 10}));
//
//        r /= 10;
//        REQUIRE((r == point2i32 {11, 11}));
//    }
//
//}

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
