#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "hash.hpp"

TEST_CASE("djb2_hash") {
    using namespace boken;

    constexpr uint32_t t           {djb2_hash_32c("t")};
    constexpr uint32_t te          {djb2_hash_32c("te")};
    constexpr uint32_t tes         {djb2_hash_32c("tes")};
    constexpr uint32_t test        {djb2_hash_32c("test")};
    constexpr uint32_t test_       {djb2_hash_32c("test_")};
    constexpr uint32_t test_string {djb2_hash_32c("test_string")};

    REQUIRE(t           == uint32_t {    177689});
    REQUIRE(te          == uint32_t {   5863838});
    REQUIRE(tes         == uint32_t { 193506769});
    REQUIRE(test        == uint32_t {2090756197});
    REQUIRE(test_       == uint32_t { 275477860});
    REQUIRE(test_string == uint32_t {4175666075});

    REQUIRE(t           == djb2_hash_32("t"));
    REQUIRE(te          == djb2_hash_32("te"));
    REQUIRE(tes         == djb2_hash_32("tes"));
    REQUIRE(test        == djb2_hash_32("test"));
    REQUIRE(test_       == djb2_hash_32("test_"));
    REQUIRE(test_string == djb2_hash_32("test_string"));
}

#endif // !defined(BK_NO_TESTS)
