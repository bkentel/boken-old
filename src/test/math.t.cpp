#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "math.hpp"

#include <vector>
#include <algorithm>

namespace bk = ::boken;

TEST_CASE("points around rect") {
    using namespace boken;

    std::vector<point2i> points;
    std::vector<point2i> expected;

    auto const push_back = [&](point2i const p) {
        points.push_back(p);
    };

    auto const is_equal = [&] {
        REQUIRE(points.size() == expected.size());

        std::sort(begin(points), end(points)
          , [](point2i const p, point2i const q) noexcept {
                return (p.y < q.y) || (p.y == q.y && p.x < q.x);
            });

        return std::equal(begin(points),   end(points)
                        , begin(expected), end(expected));
    };

    SECTION("0") {
        expected = {
            {0, 0}
        };

        points_around(point2i {0, 0}, 0, push_back);
        REQUIRE(is_equal());
    }

    SECTION("1") {
        expected = {
            {0, 0}, {1, 0}, {2, 0}
          , {0, 1},         {2, 1}
          , {0, 2}, {1, 2}, {2, 2}
        };

        points_around(point2i {1, 1}, 1, push_back);
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

        points_around(point2i {2, 2}, 2, push_back);
        REQUIRE(is_equal());
    }
}

TEST_CASE("axis_aligned_rect") {
    SECTION("from points") {
        constexpr bk::axis_aligned_rect<int> r {
            bk::offset_type_x<int> {1}
          , bk::offset_type_y<int> {2}
          , bk::offset_type_x<int> {3}
          , bk::offset_type_y<int> {4}
        };

        REQUIRE(r.x0 == 1);
        REQUIRE(r.y0 == 2);
        REQUIRE(r.x1 == 3);
        REQUIRE(r.y1 == 4);
        REQUIRE(r.width()  == 2);
        REQUIRE(r.height() == 2);
    }

    SECTION("from point + size") {
        constexpr bk::axis_aligned_rect<int> r {
            bk::offset_type_x<int> {1}
          , bk::offset_type_y<int> {2}
          , bk::size_type_x<int>   {3}
          , bk::size_type_y<int>   {4}
        };

        REQUIRE(r.x0 == 1);
        REQUIRE(r.y0 == 2);
        REQUIRE(r.x1 == (r.x0 + 3));
        REQUIRE(r.y1 == (r.y0 + 4));
        REQUIRE(r.width()  == 3);
        REQUIRE(r.height() == 4);
    }
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

    constexpr auto bounds = recti {
        offix {1},  offiy {2}
      , sizeix {5}, sizeiy {10}
    };

    SECTION("clamp with self is self") {
        REQUIRE(clamp(bounds, bounds) == bounds);
    }

    SECTION("clamp with a larger rect is bounds") {
        constexpr auto r = clamp(
            recti {
                offix {0},  offiy {0}
              , sizeix {10}, sizeiy {20}}
          , bounds);

        REQUIRE(r == bounds);
    }

    SECTION("clamp with a contained rect is the rect") {
        constexpr auto r = recti {
            offix {2},  offiy {3}
          , sizeix {3}, sizeiy {3}};

        constexpr auto r0 = clamp(r, bounds);

        REQUIRE(r == r0);
    }
}

TEST_CASE("for_each_xy") {
    using namespace boken;

    auto const r = recti {offix {1}, offiy {2}, sizeix {10}, sizeiy {5}};

    for_each_xy(r, [](point2i) {
    });

    for_each_xy(r, [](point2i, bool) {
    });
}

#endif // !defined(BK_NO_TESTS)
