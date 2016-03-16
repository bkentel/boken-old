#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "unicode.hpp"
#include "config.hpp" // string_view

#include <algorithm>
#include <vector>
#include <numeric>

TEST_CASE("unicode (ascii)") {
    boken::string_view s {"test"};
    boken::utf8_decoder_iterator decoder {s.data()};
    REQUIRE(!!decoder);

    std::vector<uint32_t> result;
    std::copy(begin(decoder), end(decoder), back_inserter(result));

    auto       it1  = begin(s);
    auto const end1 = end(s);
    auto       it2  = begin(result);
    auto const end2 = end(result);

    for ( ;it1 != end1 && it2 != end2; ++it1, ++it2) {
        auto const a = static_cast<uint8_t>(*it1);
        auto const b = *it2;
        REQUIRE(a == b);
    }

    REQUIRE(it1 == end1);
    REQUIRE(it2 == end2);
}

TEST_CASE("unicode single kanji") {
    boken::string_view s {"\xE4\xBA\x9C"};
    boken::utf8_decoder_iterator decoder {s.data()};
    REQUIRE(!!decoder);

    auto const cp = *decoder;
    REQUIRE(cp == 0x4E9C);
}

#endif // !defined(BK_NO_TESTS)
