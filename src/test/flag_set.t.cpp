#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "flag_set.hpp"

namespace boken {

struct tag_my_flags {
    static constexpr size_t size = 10;
    using type = uint32_t;
};

} // namespace boken

TEST_CASE("flag_set") {
    using namespace boken;

    constexpr auto flag0  = flag_t<tag_my_flags, 0>    {};
    constexpr auto flag1  = flag_t<tag_my_flags, 1>    {};
    constexpr auto flag2  = flag_t<tag_my_flags, 2>    {};
    constexpr auto flag01 = flag_t<tag_my_flags, 0, 1> {};

    static_assert(sizeof(flag_set<tag_my_flags>)
               == sizeof(tag_my_flags::type), "");

    // default constructed should have all flags clear
    flag_set<tag_my_flags> flags;

    REQUIRE(flags.test(flag0)  == false);
    REQUIRE(flags.test(flag1)  == false);
    REQUIRE(flags.test(flag01) == false);

    // set a multi bit flag
    flags.set(flag01);

    REQUIRE(flags.test(flag0)  == true);
    REQUIRE(flags.test(flag1)  == true);
    REQUIRE(flags.test(flag01) == true);

    REQUIRE_FALSE(flags == flag0);
    REQUIRE_FALSE(flags == flag1);
    REQUIRE(flags == flag01);

    // construct from flags
    auto const flags2 = flag0 | flag1;
    REQUIRE(flags == flags2);

    auto const flags3 = ~flags2;

    REQUIRE(flags.exclusive_any(flag01));
    REQUIRE_FALSE(flags3.exclusive_any(flag01));

    auto const flags4 = flag0 | flag2;
    REQUIRE_FALSE(flags3.exclusive_any(flag0));
    REQUIRE_FALSE(flags3.exclusive_any(flag01));
}

#endif // !defined(BK_NO_TESTS)
