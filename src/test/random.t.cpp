#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "random.hpp"

#include <algorithm>
#include <vector>
#include <cstdint>

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
