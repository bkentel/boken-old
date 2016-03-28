#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "circular_buffer.hpp"

#include <string>

TEST_CASE("circular_buffer") {
    using namespace boken;

    constexpr size_t capacity = 10;

    simple_circular_buffer<std::string> buffer {capacity};

    //
    // initial conditions
    //
    REQUIRE(buffer.size() == 0);

    REQUIRE(begin(buffer) == begin(buffer));
    REQUIRE(end(buffer)   == end(buffer));
    REQUIRE(begin(buffer) == end(buffer));
    REQUIRE(end(buffer)   == begin(buffer));

    REQUIRE(std::distance(begin(buffer), end(buffer)) == 0);

    //
    // fill the buffer by one more than its capactiy
    //
    std::array<char const*, capacity + 1> strings {
        "test0"
      , "test1"
      , "test2"
      , "test3"
      , "test4"
      , "test5"
      , "test6"
      , "test7"
      , "test8"
      , "test9"
      , "testA"
    };

    for (size_t i = 0; i < strings.size(); ++i) {
        auto const size_before = buffer.size();
        buffer.push(strings[i]);
        auto const size_after = buffer.size();;

        if (i < capacity) {
            REQUIRE((size_after - size_before) == 1);
        } else {
            REQUIRE((size_after - size_before) == 0);
        }

        auto const& b = buffer; // via const iterators
        REQUIRE(std::distance(begin(b), end(b)) == static_cast<ptrdiff_t>(size_after));

        for (size_t j = 0; j < buffer.size(); ++j) {
            auto const k = j + (i / buffer.size());
            REQUIRE(buffer[j] == strings[k]);
        }
    }

    REQUIRE(buffer.size() == 10);

    //
    // element order
    //
    {
        auto it = begin(buffer);
        for (size_t i = 0; i < buffer.size(); ++i, ++it) {
            REQUIRE(*it == strings[i + 1]);
        }
    }

    //
    // relative indexing
    //

    // front == 1

    REQUIRE(buffer[-9] == buffer[ 1]);
    REQUIRE(buffer[ 9] == buffer[-1]);

    REQUIRE(buffer[-1] == strings[10]);
    REQUIRE(buffer[ 0] == strings[1]);
    REQUIRE(buffer[ 1] == strings[2]);

}

#endif // !defined(BK_NO_TESTS)
