#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "random.hpp"
#include "random_algorithm.hpp"
#include "utility.hpp"

#include <algorithm>
#include <numeric>
#include <vector>
#include <cstdint>

TEST_CASE("for_each_xy_random") {
    using namespace boken;

    auto const state = make_random_state();
    auto& rng = *state;

    std::array<std::array<int, 8>, 10> result {};

    auto const r = recti32 {point2i32 {}, sizei32x {10}, sizei32y {8}};
    int i = 0;
    for_each_xy_random(rng, r, [&](point2i32 const p) noexcept {
        auto const x = value_cast_unsafe<size_t>(p.x);
        auto const y = value_cast_unsafe<size_t>(p.y);

        result[x][y] = ++i;
    });

    REQUIRE(i == (10 * 8));

    auto const n = std::accumulate(begin(result), end(result), ptrdiff_t {}
      , [](auto const sum, auto const& row) noexcept {
            return sum + std::count_if(begin(row), end(row)
              , [](int const m) noexcept {
                    return m != 0;
                });
        });

    REQUIRE(n == (10 * 8));
}

TEST_CASE("random weighted") {
    using namespace boken;

    weight_list<int, int> const weights {
        {1, 0}
      , {3, 1}
      , {5, 2}
      , {3, 3}
      , {1, 4}
    };

    auto const state = make_random_state();
    auto& rng = *state;

    random_weighted(rng, weights);
}

TEST_CASE("random_chance_in_x") {
    using namespace boken;

    auto const state = make_random_state();
    auto& rng = *state;

    SECTION("1 in 1 chance") {
        auto n = 0;

        for (int i = 0; i < 100; ++i) {
            n += random_chance_in_x(rng, 1, 1) ? 1 : 0;
        }

        REQUIRE(n == 100);
    }

    SECTION("1 in 2 chance") {
        constexpr int iterations = 3000;
        constexpr int expected   = iterations / 2;
        constexpr int tolerence  = iterations / 100; // 1%

        auto n = 0;

        for (int i = 0; i < iterations; ++i) {
            n += random_chance_in_x(rng, 1, 2) ? 1 : 0;
        }

        auto const delta = std::abs(n - expected);
        REQUIRE(delta < tolerence);
    }

    SECTION("5 in 10 chance") {
        constexpr int iterations = 3000;
        constexpr int expected   = iterations / 2;
        constexpr int tolerence  = iterations / 100; // 1%

        auto n = 0;

        for (int i = 0; i < iterations; ++i) {
            n += random_chance_in_x(rng, 5, 10) ? 1 : 0;
        }

        auto const delta = std::abs(n - expected);
        REQUIRE(delta < tolerence);
    }

    SECTION("9 in 10 chance") {
        constexpr int iterations = 3000;
        constexpr int expected   = iterations * 9 / 10;
        constexpr int tolerence  = iterations / 100; // 1%

        auto n = 0;

        for (int i = 0; i < iterations; ++i) {
            n += random_chance_in_x(rng, 9, 10) ? 1 : 0;
        }

        auto const delta = std::abs(n - expected);
        REQUIRE(delta < tolerence);
    }
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
