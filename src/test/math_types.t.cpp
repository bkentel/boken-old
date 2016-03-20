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

} //namespace

TEST_CASE("basic_1_tuple", "[math]") {
    using namespace boken;

    SECTION("construction") {
        sizei16 const s0 = int16_t {1}; // from constant
        sizei32 const t0 = int16_t {2}; // from smaller constant type
        sizei32 const s1 = s0;          // from smaller type
        sizei16 const t1 = s0;          // from same size type

        REQUIRE(check_value_type<int16_t>(s0) == 1);
        REQUIRE(check_value_type<int32_t>(t0) == 2);
        REQUIRE(check_value_type<int32_t>(s1) == 1);
        REQUIRE(check_value_type<int16_t>(t1) == 1);
    }

    SECTION("comparison") {
        sizei16 const u = int16_t {1};
        sizei32 const v = int32_t {2};
        sizei32 const w = int32_t {1};

        // self equality
        REQUIRE(u == u);
        REQUIRE(v == v);
        REQUIRE(w == w);

        // self unequality
        REQUIRE_FALSE(u != u);
        REQUIRE_FALSE(v != v);
        REQUIRE_FALSE(w != w);

        // self inequality
        REQUIRE_FALSE(u < u);
        REQUIRE_FALSE(v < v);
        REQUIRE_FALSE(w < w);

        REQUIRE_FALSE(u > u);
        REQUIRE_FALSE(v > v);
        REQUIRE_FALSE(w > w);

        REQUIRE(u <= u);
        REQUIRE(v <= v);
        REQUIRE(w <= w);

        REQUIRE(u >= u);
        REQUIRE(v >= v);
        REQUIRE(w >= w);

        // equality with diff. underlying types
        REQUIRE(u == w);
        REQUIRE(w == u);

        // unequality with diff. underlying types
        REQUIRE(u != v);
        REQUIRE(v != u);

        REQUIRE(v != w);
        REQUIRE(w != v);

        // inequalities between diff. underlying types
        REQUIRE(u <  v);
        REQUIRE(u <= v);

        REQUIRE(v >  u);
        REQUIRE(v >= u);
    }

    SECTION("arithmetic") {
        offi16x  const p = int16_t {1};
        offi32x  const q = int32_t {2};
        sizei16x const u = int16_t {3};
        sizei32x const v = int32_t {4};

        // vector + vector
        REQUIRE(check_value_type<int32_t>(p + q) == 3);
        REQUIRE(check_value_type<int32_t>(q + p) == 3);

        // vector - vector
        REQUIRE(check_value_type<int32_t>(p - q) == -1);
        REQUIRE(check_value_type<int32_t>(q - p) ==  1);

        // point + vector
        REQUIRE(check_value_type<int32_t>(u + q) == 5);

        // point - vector
        REQUIRE(check_value_type<int32_t>(u - q) == 1);

        // point - point
        REQUIRE(check_value_type<int32_t>(v - u) == 1);

        // vector * constant
        REQUIRE(check_value_type<int16_t>(u * int16_t {2}) == 6);
        REQUIRE(check_value_type<int32_t>(v * int32_t {2}) == 8);

        // point * constant
        REQUIRE(check_value_type<int16_t>(p * int16_t {2}) == 2);
        REQUIRE(check_value_type<int32_t>(q * int32_t {2}) == 4);

        // vector / constant
        REQUIRE(check_value_type<int16_t>(u / int16_t {3}) == 1);
        REQUIRE(check_value_type<int32_t>(v / int32_t {2}) == 2);

        // point / constant
        REQUIRE(check_value_type<int16_t>(p / int16_t {1}) == 1);
        REQUIRE(check_value_type<int32_t>(q / int32_t {2}) == 1);

        // mutating assignment vector & vector
        offi32x x = 1;

        x += p;
        REQUIRE(value_cast(x) == 2);

        x -= q;
        REQUIRE(value_cast(x) == 0);

        x *= 10;
        REQUIRE(value_cast(x) == 0);

        x /= 10;
        REQUIRE(value_cast(x) == 0);

        // mutating assignment point & vector
        sizei32x y = 1;

        y += p;
        REQUIRE(value_cast(y) == 2);

        y -= q;
        REQUIRE(value_cast(y) == 0);

        y *= 10;
        REQUIRE(value_cast(y) == 0);

        y /= 10;
        REQUIRE(value_cast(y) == 0);

    }

}

TEST_CASE("basic_2_tuple", "[math]") {
    using namespace boken;

    SECTION("construction") {
        point2i16 const p0 {int16_t {1}, int16_t {1}}; // from constant
        point2i32 const q0 {int16_t {2}, int16_t {2}}; // from smaller constant type
        point2i32 const p1 = p0;                       // from smaller type
        point2i16 const q1 = p0;                       // from same size type

        REQUIRE(check_value_type_xy<int16_t>(p0)
             == std::make_pair(int16_t {1}, int16_t {1}));
        REQUIRE(check_value_type_xy<int32_t>(q0) == std::make_pair(2, 2));
        REQUIRE(check_value_type_xy<int32_t>(p1) == std::make_pair(1, 1));
        REQUIRE(check_value_type_xy<int16_t>(q1)
             == std::make_pair(int16_t {1}, int16_t {1}));
    }

    SECTION("comparison") {
        point2i16 const p = {int16_t {1}, int16_t {1}};
        point2i32 const q = {int32_t {2}, int16_t {1}};
        point2i32 const r = {int32_t {1}, int16_t {1}};
        vec2i16   const u = {int16_t {1}, int16_t {1}};
        vec2i32   const v = {int32_t {2}, int16_t {1}};
        vec2i32   const w = {int32_t {1}, int16_t {1}};

        // self equality
        REQUIRE(p == p);
        REQUIRE(q == q);
        REQUIRE(r == r);

        REQUIRE(u == u);
        REQUIRE(v == v);
        REQUIRE(w == w);

        // self unequality
        REQUIRE_FALSE(p != p);
        REQUIRE_FALSE(q != q);
        REQUIRE_FALSE(r != r);

        REQUIRE_FALSE(u != u);
        REQUIRE_FALSE(v != v);
        REQUIRE_FALSE(w != w);

        // equality with diff. underlying types
        REQUIRE(p == r);
        REQUIRE(r == p);

        REQUIRE(u == w);
        REQUIRE(w == u);

        // unequality with diff. underlying types
        REQUIRE(p != r);
        REQUIRE(r != p);

        REQUIRE(u != w);
        REQUIRE(w != u);
    }

    SECTION("arithmetic") {
        point2i16 const p = {int16_t {1}, int16_t {1}};
        point2i32 const q = {int32_t {2}, int16_t {1}};
        vec2i16   const u = {int16_t {1}, int16_t {1}};
        vec2i32   const v = {int32_t {2}, int16_t {1}};

        // vector + vector
        REQUIRE(check_value_type_xy<int32_t>(u + v)
             == std::make_pair<int32_t>(3, 1));

        // vector - vector
        REQUIRE(check_value_type_xy<int32_t>(u - v)
             == std::make_pair<int32_t>(-1, 0));

        // point + vector
        REQUIRE(check_value_type_xy<int32_t>(p + v)
             == std::make_pair<int32_t>(3, 2));

        // point - vector
        REQUIRE(check_value_type_xy<int32_t>(q - u)
             == std::make_pair<int32_t>(1, 0));

        // point - point
        REQUIRE(check_value_type_xy<int32_t>(p - q)
             == std::make_pair<int32_t>(-1, 0));

        // vector * constant
        REQUIRE(check_value_type_xy<int32_t>(u * 1)
             == std::make_pair<int32_t>(1, 1));

        // point * constant
        REQUIRE(check_value_type_xy<int32_t>(p * 1)
             == std::make_pair<int32_t>(1, 1));

        // vector / constant
        REQUIRE(check_value_type_xy<int32_t>(u / 1)
             == std::make_pair<int32_t>(1, 1));

        // point / constant
        REQUIRE(check_value_type_xy<int32_t>(p / 1)
             == std::make_pair<int32_t>(1, 1));

        // mutating assignment vector & vector
        vec2i32 w = {1, 1};

        w += v;
        REQUIRE((w == vec2i32 {3, 2}));

        w -= v;
        REQUIRE((w == vec2i32 {1, 1}));

        w *= 10;
        REQUIRE((w == vec2i32 {10, 10}));

        w /= 10;
        REQUIRE((w == vec2i32 {1, 1}));

        // mutating assignment point & vector
        point2i32 r = {1, 1};

        r += u;
        REQUIRE((r == point2i32 {2, 2}));

        r -= u;
        REQUIRE((r == point2i32 {1, 1}));

        r *= 10;
        REQUIRE((r == point2i32 {10, 10}));

        r /= 10;
        REQUIRE((r == point2i32 {11, 11}));
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
}

#endif // !defined(BK_NO_TESTS)
