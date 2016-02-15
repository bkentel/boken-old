#if !defined(BK_NO_TESTS)
#include "catch.hpp"

#include "bsp_generator.hpp"
#include "random.hpp"

namespace bk = ::boken;

TEST_CASE("bsp_generator") {
    using bk::bsp_generator;

    auto  rng_state = bk::make_random_state();
    auto& rng = *rng_state;

    auto const is_empty = [](bsp_generator const& bsp, bool const empty) {
        REQUIRE((bsp.size() == 0) == empty);
        REQUIRE((bsp.empty()) == empty);
        REQUIRE((bsp.begin() == bsp.end()) == empty);
        return true;
    };

    SECTION("default params") {
        bsp_generator::param_t p;
        auto bsp = bk::make_bsp_generator(p);

        REQUIRE(is_empty(*bsp, true));
        bsp->generate(rng);
        REQUIRE(is_empty(*bsp, false));
        bsp->clear();
        REQUIRE(is_empty(*bsp, true));
    }
}

#endif // !defined(BK_NO_TESTS)
