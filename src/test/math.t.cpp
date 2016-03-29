#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "math.hpp"

#include <vector>
#include <algorithm>

namespace bk = ::boken;

namespace {

void arity_0() {};
void arity_1(int) {};
void arity_2(int, int) {};

} //namespace

TEST_CASE("arity_of") {
    using namespace boken;

    // suppress warnings
    arity_0();
    arity_1(0);
    arity_2(0, 0);

    // free functions
    static_assert(arity_of<decltype(arity_0)>::value == 0, "");
    static_assert(arity_of<decltype(arity_1)>::value == 1, "");
    static_assert(arity_of<decltype(arity_2)>::value == 2, "");

    // free function pointers
    static_assert(arity_of<decltype(&arity_0)>::value == 0, "");
    static_assert(arity_of<decltype(&arity_1)>::value == 1, "");
    static_assert(arity_of<decltype(&arity_2)>::value == 2, "");

    // lambdas convertible to free functions
    auto const lambda_0 = []() {};
    auto const lambda_1 = [](int) {};
    auto const lambda_2 = [](int, int) {};

    static_assert(arity_of<decltype(lambda_0)>::value == 0, "");
    static_assert(arity_of<decltype(lambda_1)>::value == 1, "");
    static_assert(arity_of<decltype(lambda_2)>::value == 2, "");

    // capturing lambdas
    int i {};
    auto const cap_lambda_0 = [&]() { ++i; };
    auto const cap_lambda_1 = [&](int) { ++i; };
    auto const cap_lambda_2 = [&](int, int) { ++i; };

    static_assert(arity_of<decltype(cap_lambda_0)>::value == 0, "");
    static_assert(arity_of<decltype(cap_lambda_1)>::value == 1, "");
    static_assert(arity_of<decltype(cap_lambda_2)>::value == 2, "");

    // member functions
    struct foo {
        void member_0() {}
        void member_1(int) {}
        void member_2(int, int) {}
    };

    // suppress warnings
    foo {}.member_0();
    foo {}.member_1(0);
    foo {}.member_2(0, 0);

    static_assert(arity_of<decltype(&foo::member_0)>::value == 0, "");
    static_assert(arity_of<decltype(&foo::member_1)>::value == 1, "");
    static_assert(arity_of<decltype(&foo::member_2)>::value == 2, "");
}

TEST_CASE("clamp") {
    constexpr int lo = 1;
    constexpr int hi = 10;

    REQUIRE(bk::clamp(lo - 1, lo, hi) == lo    );
    REQUIRE(bk::clamp(lo    , lo, hi) == lo    );
    REQUIRE(bk::clamp(lo + 1, lo, hi) == lo + 1);

    REQUIRE(bk::clamp(hi - 1, lo, hi) == hi - 1);
    REQUIRE(bk::clamp(hi    , lo, hi) == hi    );
    REQUIRE(bk::clamp(hi + 1, lo, hi) == hi    );
}

TEST_CASE("clamp rect") {
    using namespace boken;

    constexpr auto bounds = recti32 {
        offi32x  {1},  offi32y {2}
      , sizei32x {5}, sizei32y {10}
    };

    SECTION("clamp with self is self") {
        REQUIRE(clamp(bounds, bounds) == bounds);
    }

    SECTION("clamp with a larger rect is bounds") {
        constexpr auto r = clamp(
            recti32 {offi32x  {0},  offi32y {0}
                  , sizei32x {10}, sizei32y {20}}
          , bounds);

        REQUIRE(r == bounds);
    }

    SECTION("clamp with a contained rect is the rect") {
        constexpr auto r = recti32 {
            offi32x  {2},  offi32y {3}
          , sizei32x {3}, sizei32y {3}};

        constexpr auto r0 = clamp(r, bounds);

        REQUIRE(r == r0);
    }
}

TEST_CASE("for_each_xy") {
    using namespace boken;

    auto const r = recti32 {
        offi32x {1}, offi32y {2}, sizei32x {10}, sizei32y {5}};

    for_each_xy(r, [](point2i32) {
    });

    for_each_xy(r, [](point2i32, bool) {
    });
}

TEST_CASE("points around rect") {
    using namespace boken;

    std::vector<point2i32> points;
    std::vector<point2i32> expected;

    auto const push_back = [&](point2i32 const p) {
        points.push_back(p);
    };

    auto const is_equal = [&] {
        REQUIRE(points.size() == expected.size());

        std::sort(begin(points), end(points)
          , [](point2i32 const p, point2i32 const q) noexcept {
                return (p.y < q.y) || (p.y == q.y && p.x < q.x);
            });

        return std::equal(begin(points),   end(points)
                        , begin(expected), end(expected));
    };

    SECTION("0") {
        expected = {
            {0, 0}
        };

        points_around(point2i32 {0, 0}, 0, push_back);
        REQUIRE(is_equal());
    }

    SECTION("1") {
        expected = {
            {0, 0}, {1, 0}, {2, 0}
          , {0, 1},         {2, 1}
          , {0, 2}, {1, 2}, {2, 2}
        };

        points_around(point2i32 {1, 1}, 1, push_back);
        REQUIRE(is_equal());
    }

    SECTION("2") {
        expected = {
            {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}
          , {0, 1},                         {4, 1}
          , {0, 2},                         {4, 2}
          , {0, 3},                         {4, 3}
          , {0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4}
        };

        points_around(point2i32 {2, 2}, 2, push_back);
        REQUIRE(is_equal());
    }
}

#endif // !defined(BK_NO_TESTS)
