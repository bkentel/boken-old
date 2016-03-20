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

}

TEST_CASE("basic_1_tuple", "[math]") {
    using namespace boken;

    SECTION("construction") {
        sizei16 const u0 = int16_t {1}; // from constant
        sizei32 const v0 = int16_t {2}; // from smaller constant type
        sizei16 const u1 = u0;          // from smaller type
        sizei32 const v1 = u0;          // from same size type

        REQUIRE(check_value_type<int16_t>(u0) == 1);
        REQUIRE(check_value_type<int32_t>(v0) == 2);
        REQUIRE(check_value_type<int16_t>(u1) == 1);
        REQUIRE(check_value_type<int32_t>(v1) == 1);
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
