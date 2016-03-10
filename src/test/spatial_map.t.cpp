#if !defined(BK_NO_TESTS)
#include "catch.hpp"

#include "spatial_map.hpp"

TEST_CASE("spatial map") {
    using namespace boken;

    struct key_t {
        float operator()(float const f) const noexcept {
            return f;
        }
    };

    struct property_t {
        int operator()(float const f) const noexcept {
            return f < 0.0f ? -1 : 1;
        }
    };

    constexpr int16_t width  = 20;
    constexpr int16_t height = 10;
    spatial_map<float, key_t, property_t, int16_t> map {width, height};

    REQUIRE(map.size() == 0);

    // insert 3 new unique values
    REQUIRE(map.insert({1, 2}, 2.0f));
    REQUIRE(map.insert({1, 1}, 1.0f));
    REQUIRE(map.insert({2, 1}, 3.0f));

    // insert 3 duplicated
    REQUIRE(!map.insert({1, 2}, 2.0f));
    REQUIRE(!map.insert({1, 1}, 1.0f));
    REQUIRE(!map.insert({2, 1}, 3.0f));

    // update an existing value
    REQUIRE(!map.insert_or_replace({2, 1}, 4.0f));
    REQUIRE(!!map.find({2, 1}));
    REQUIRE(*map.find({2, 1}) == 4.0f);

    REQUIRE(map.size() == 3);

    {
        auto const range = map.positions_range();
        REQUIRE(std::distance(range.first, range.second) == 3);
    }

    {
        auto const range = map.properties_range();
        REQUIRE(std::distance(range.first, range.second) == 3);
    }

    // find by pos
    REQUIRE(!!map.find({1, 1}));
    REQUIRE(*map.find({1, 1}) == 1.0f);

    // find by key
    REQUIRE(!!map.find(1.0f));
    REQUIRE(*map.find(1.0f) == 1.0f);

    // erase invalid by pos
    REQUIRE(!map.erase({0, 0}));
    REQUIRE(map.size() == 3);

    // erase invalid by key
    REQUIRE(!map.erase(0.0f));
    REQUIRE(map.size() == 3);

    // erase by pos
    REQUIRE(map.erase({1, 1}));
    REQUIRE(map.size() == 2);

    // erase by key
    REQUIRE(map.erase(4.0f));
    REQUIRE(map.size() == 1);
}

#endif // !defined(BK_NO_TESTS)
