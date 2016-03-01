#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "entity.hpp"
#include "entity_def.hpp"

#include <algorithm>
#include <array>
#include <vector>

TEST_CASE("property_set") {
    using namespace boken;

    enum class test_enum {
        a, b, c, d, e
    };

    using pair_t = std::pair<test_enum, char>;
    property_set<test_enum, char> properties;

    REQUIRE(properties.size() == 0u);
    REQUIRE(properties.empty());
    REQUIRE(properties.begin() == properties.end());

    REQUIRE(5 == properties.add_or_update_properties({
        {test_enum::e, 'e'}
      , {test_enum::d, 'd'}
      , {test_enum::c, 'c'}
      , {test_enum::b, 'b'}
      , {test_enum::a, 'a'}
    }));

    REQUIRE(properties.size() == 5u);
    REQUIRE(!properties.empty());
    REQUIRE(std::distance(properties.begin(), properties.end()) == 5);

    REQUIRE(properties.value_or(test_enum::a, '\0') == 'a');
    REQUIRE(properties.value_or(test_enum::b, '\0') == 'b');
    REQUIRE(properties.value_or(test_enum::c, '\0') == 'c');
    REQUIRE(properties.value_or(test_enum::d, '\0') == 'd');
    REQUIRE(properties.value_or(test_enum::e, '\0') == 'e');

    SECTION("remove values") {
        auto size = properties.size();

        auto const remove = [&](test_enum const e) {
            return properties.has_property(e)
                && properties.remove_property(e)
                && (properties.size() == --size)
                && (properties.value_or(e, '\0') == '\0');
        };

        REQUIRE(remove(test_enum::a));
        REQUIRE(remove(test_enum::d));
        REQUIRE(remove(test_enum::b));
        REQUIRE(remove(test_enum::e));
        REQUIRE(remove(test_enum::c));

        REQUIRE(properties.empty());
        REQUIRE(properties.size() == 0);
    }

    SECTION("insert duplicates") {
        REQUIRE(0 == properties.add_or_update_properties({
            {test_enum::e, 'f'}
          , {test_enum::d, 'e'}
          , {test_enum::c, 'd'}
          , {test_enum::b, 'c'}
          , {test_enum::a, 'b'}
        }));

        REQUIRE(properties.size() == 5u);

        REQUIRE(properties.value_or(test_enum::a, '\0') == 'b');
        REQUIRE(properties.value_or(test_enum::b, '\0') == 'c');
        REQUIRE(properties.value_or(test_enum::c, '\0') == 'd');
        REQUIRE(properties.value_or(test_enum::d, '\0') == 'e');
        REQUIRE(properties.value_or(test_enum::e, '\0') == 'f');
    }

}


#endif // !defined(BK_NO_TESTS)
