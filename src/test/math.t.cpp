#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "math.hpp"

#include <vector>
#include <algorithm>

namespace bk = ::boken;

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


#endif // !defined(BK_NO_TESTS)
