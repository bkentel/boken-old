#if !defined(BK_NO_TESTS)
#include "catch.hpp"

#include "bsp_generator.hpp"
#include "random.hpp"

namespace bk = ::boken;

TEST_CASE("bsp_generator") {
    auto  rng_state = bk::make_random_state();
    auto& rng = *rng_state;

    SECTION("default params") {
        bk::bsp_generator::param_t p;
        auto bsp = bk::make_bsp_generator(p);

        REQUIRE(bsp->size() == 0);
        REQUIRE(bsp->empty());
        REQUIRE(bsp->begin() == bsp->end());

        bsp->generate(rng);

        REQUIRE(bsp->size() > 0);
        REQUIRE(!bsp->empty());
        REQUIRE(bsp->begin() != bsp->end());
    }
}

#endif // !defined(BK_NO_TESTS)
