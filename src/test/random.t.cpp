#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "random.hpp"
#include "utility.hpp"

#include <algorithm>
#include <vector>
#include <cstdint>

TEST_CASE("random weighted") {
    using namespace boken;

    weight_list<int, int> const weights {
        {400, 1000}
      , {100, 800}
      , {50,  400}
      , {25,  150}
      , {0,   100}
    };

    // min max values
    REQUIRE(weights.max_key() == 400);
    REQUIRE(weights.min_key() == 0);
    REQUIRE(weights.max_val() == 1000);
    REQUIRE(weights.min_val() == 100);

    // direct lookup
    REQUIRE(weights[-1].second == 100);
    REQUIRE(weights[ 0].second == 100);
    REQUIRE(weights[24].second == 100);

    REQUIRE(weights[25].second == 150);
    REQUIRE(weights[49].second == 150);

    REQUIRE(weights[399].second == 800);
    REQUIRE(weights[400].second == 1000);
    REQUIRE(weights[401].second == 1000);

    auto const state = make_random_state();
    auto& rng = *state;

    random_weighted(rng, weights);
}

TEST_CASE("random") {
    using namespace boken;

    auto const state = make_random_state();
    auto& rng = *state;

    constexpr size_t n = 100;
    std::vector<int> result;
    result.reserve(n);

    SECTION("coin flip") {
        std::generate_n(back_inserter(result), n, [&]() noexcept {
            return random_coin_flip(rng) ? 1 : 0;
        });

        std::vector<int> const expected {
            0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0
          , 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0
          , 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1
          , 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1
          , 1, 1, 1, 1, 1, 1, 1, 1 };

        REQUIRE(std::equal(begin(result),   end(result)
                         , begin(expected), end(expected)));
    }

    SECTION("chance in x") {
        std::generate_n(back_inserter(result), 100, [&]() noexcept {
            return random_chance_in_x(rng, 1, 25);
        });

        std::vector<int> const expected {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
          , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
          , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
          , 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
          , 0, 0, 0, 0, 0, 0, 0, 0, };

        REQUIRE(std::equal(begin(result),   end(result)
                         , begin(expected), end(expected)));
    }
}

#endif // !defined(BK_NO_TESTS)
