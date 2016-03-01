#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "utility.hpp"

#include <algorithm>
#include <array>
#include <vector>

TEST_CASE("sub_region_iterator") {
    using namespace boken;

    constexpr int w = 5;
    constexpr int h = 4;

    std::vector<int> const v {
        0,   1,  2,  3,  4
      , 10, 11, 12, 13, 14
      , 20, 21, 22, 23, 24
      , 30, 31, 32, 33, 34
    };

    REQUIRE(v.size() == static_cast<size_t>(w * h));

    SECTION("fully contained sub region") {
        constexpr int offx = 1;
        constexpr int offy = 1;
        constexpr int sw   = 3;
        constexpr int sh   = 2;

        auto const p = make_sub_region_range(v.data(), offx, offy, w, h, sw, sh);
        auto it      = p.first;
        auto last    = p.second;

        REQUIRE((last - it) == 6);

        std::array<int, 6> const expected {
            11, 12, 13
          , 21, 22, 23
        };

        std::vector<int> actual;
        std::copy(it, last, back_inserter(actual));

        REQUIRE(std::equal(begin(expected), end(expected)
                         , begin(actual),   end(actual)));

    }

    SECTION("rebound fully contained sub region") {
        constexpr int offx = 1;
        constexpr int offy = 1;
        constexpr int sw   = 2;
        constexpr int sh   = 2;

        std::vector<char> const v0 {
            'a', 'a', 'a', 'a', 'a'
          , 'a', 'B', 'C', 'a', 'a'
          , 'a', 'D', 'E', 'a', 'a'
          , 'a', 'a', 'a', 'a', 'a'
        };

        auto const p = make_sub_region_range(v.data(), offx, offy, w, h, sw, sh);
        auto it      = const_sub_region_iterator<char> {p.first,  v0.data()};
        auto last    = const_sub_region_iterator<char> {p.second, v0.data()};

        REQUIRE((last - it) == 4);

        std::array<int, 4> const expected {
            'B', 'C'
          , 'D', 'E'
        };

        std::vector<char> actual;
        std::copy(it, last, back_inserter(actual));
        REQUIRE(std::equal(begin(expected), end(expected)
                         , begin(actual),   end(actual)));
    }
}

#endif // !defined(BK_NO_TESTS)
