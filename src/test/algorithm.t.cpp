#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "algorithm.hpp"

#include <vector>

TEST_CASE("copy_index_if") {
    using namespace boken;

    std::vector<int> const data {
        0, 1, 2, 3, 4, 5, 6
    };

    std::vector<int> result;

    SECTION("matching the 0th item result in one match of the 0th index") {
        copy_index_if(data, 0, back_inserter(result)
          , [i = 0](auto) mutable noexcept {
                return i++ == 0;
            });

        REQUIRE(result.size() == 1u);
        REQUIRE(result[0] == 0);
    }

    SECTION("matching the last item result in one match of the size - 1 index") {
        copy_index_if(data, 0, back_inserter(result)
          , [i = data.size()](auto) mutable noexcept {
                return --i == 0u;
            });

        REQUIRE(result.size() == 1u);
        REQUIRE(result[0] == static_cast<int>(data.size() - 1));
    }
}

#endif // !defined(BK_NO_TESTS)
