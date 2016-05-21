#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "utility.hpp"
#include "maybe.hpp"

#include <algorithm>
#include <array>
#include <vector>

TEST_CASE("maybe") {
    using namespace boken;

    auto const get_empty = [] {
        return maybe<int> {nullptr};
    };

    auto const get_ok = [] {
        return maybe<int> {1};
    };

    SECTION("empty maybe calls || and not && functor") {
        bool good = false;
        bool bad  = false;

        get_empty() && [&](int) noexcept { good = true; }
                    || [&]()    noexcept { bad  = true; };

        REQUIRE(good == false);
        REQUIRE(bad  == true);
    }

    SECTION("good maybe calls && and not || functor") {
        bool good = false;
        bool bad  = false;

        get_ok() && [&](int) noexcept { good = true; }
                 || [&]()    noexcept { bad  = true; };

        REQUIRE(good == true);
        REQUIRE(bad  == false);
    }

    using vec_t = std::vector<int>;
    maybe<vec_t> {1, 2, 3}
     && [](vec_t const v) { REQUIRE(v.size() == 3u); }
     || [] { REQUIRE(false); };


    SECTION("initialized reference types are valid") {
        int a = 0;
        maybe<int&> ref = a;

        ref && [&](int& A) {
            REQUIRE(A == 0);
            A = 1;
        };

        REQUIRE(a == 1);
    }

    SECTION("uninitialized reference types are valid") {
        maybe<int&> ref {nullptr};

        bool empty = false;
        ref && [&](int&) { REQUIRE(false); }
            || [&] { empty = true; };

        REQUIRE(empty);
    }

    SECTION("initialized const reference types are valid") {
        int a = 0;
        maybe<int const&> ref = a;

        ref && [&](int const& A) { REQUIRE(A == 0); }
            || [&] { REQUIRE(false); };
    }

    SECTION("uninitialized const reference types are valid") {
        maybe<int const&> ref {nullptr};

        bool empty = false;
        ref && [&](int const&) { REQUIRE(false); }
            || [&] { empty = true; };

        REQUIRE(empty);
    }
}


TEST_CASE("weight_list") {
    using namespace boken;

    SECTION("from initializer_list") {
        weight_list<int, int> const w {
            {6, 0}
          , {3, 1}
          , {1, 2}
        };

        REQUIRE(w[0] == 0);
        REQUIRE(w[1] == 0);
        REQUIRE(w[2] == 0);
        REQUIRE(w[3] == 0);
        REQUIRE(w[4] == 0);
        REQUIRE(w[5] == 0);
        REQUIRE(w[6] == 1);
        REQUIRE(w[7] == 1);
        REQUIRE(w[8] == 1);
        REQUIRE(w[9] == 2);
    }
}

TEST_CASE("static_string_buffer") {
    using namespace boken;
    static_string_buffer<16> buffer;

    REQUIRE(buffer.capacity() == 16u);
    REQUIRE(buffer.size() == 0);
    REQUIRE(buffer.empty());
    REQUIRE(buffer.begin() == buffer.end());

    REQUIRE(buffer.append("0123456789ABCDE"));
    REQUIRE(buffer.size() == 15u);
    REQUIRE(buffer.full());
    REQUIRE(!buffer);
    REQUIRE(buffer.data()[15] == '\0');
    REQUIRE(buffer.to_string_view() == "0123456789ABCDE");

    buffer.clear();
    REQUIRE(buffer.size() == 0);
    REQUIRE(!buffer.full());
    REQUIRE(!!buffer);

    REQUIRE(!buffer.append("0123456789ABCDEF"));
    REQUIRE(buffer.size() == 15u);
    REQUIRE(buffer.full());
    REQUIRE(!buffer);
    REQUIRE(buffer.data()[15] == '\0');
    REQUIRE(buffer.to_string_view() == "0123456789ABCDE");

    buffer.clear();
    REQUIRE(buffer.size() == 0);
    REQUIRE(!buffer.full());
    REQUIRE(!!buffer);

    REQUIRE(buffer.append("%d", 123));
    REQUIRE(buffer.size() == 3u);
    REQUIRE(buffer.data()[3] == '\0');
    REQUIRE(buffer.to_string_view() == "123");
}

TEST_CASE("as_unsigned") {
    using namespace boken;

    SECTION("clamped") {
        REQUIRE(as_unsigned(char { 1}) == 1u);
        REQUIRE(as_unsigned(char {-1}) == 0u);

        REQUIRE(as_unsigned(short { 1}) == 1u);
        REQUIRE(as_unsigned(short {-1}) == 0u);

        REQUIRE(as_unsigned(int { 1}) == 1u);
        REQUIRE(as_unsigned(int {-1}) == 0u);

        REQUIRE(as_unsigned(long { 1}) == 1u);
        REQUIRE(as_unsigned(long {-1}) == 0u);

        using ll = long long;
        REQUIRE(as_unsigned(ll { 1}) == 1u);
        REQUIRE(as_unsigned(ll {-1}) == 0u);
    }
}

namespace {

// Use SFINAE to eliminate this overload if as_const is deleted, otherwise "true"
template <typename T>
decltype(boken::as_const(std::declval<T&&>())) as_const_test(T&&);
// Catch-all fallback to always false
void as_const_test(...) {}
} //namespace

TEST_CASE("as_const") {
    using namespace boken;

    int        a = 0;
    int const  b = 0;

    REQUIRE(a == 0); // unused variables
    REQUIRE(b == 0); // unused variables
    as_const_test(); // suppress warning

    using rval_a = decltype(as_const_test(std::move(a)));
    using lval_a = decltype(as_const_test(a));

    static_assert(std::is_void<rval_a>::value, "");
    static_assert(std::is_const<std::remove_reference_t<lval_a>>::value, "");

    using rval_b = decltype(as_const_test(std::move(b)));
    using lval_b = decltype(as_const_test(b));

    static_assert(std::is_void<rval_b>::value, "");
    static_assert(std::is_const<std::remove_reference_t<lval_b>>::value, "");
}

TEST_CASE("call_destructor") {
    using namespace boken;

    SECTION("fundamental type") {
        int a = 0;
        call_destructor(a);
    }

    SECTION("nothrow destructor") {
        bool destructor_called = false;

        struct test {
            test(bool& flag) noexcept : flag_ {flag} {}
            ~test() { flag_ = true; }
            bool& flag_;
        };

        test t {destructor_called};
        REQUIRE(noexcept(call_destructor(t)));
        call_destructor(t);
        REQUIRE(destructor_called);
    }

    SECTION("throwing destructor") {
        bool destructor_called = false;

        struct test {
            test(bool& flag) noexcept : flag_ {flag} {}
            ~test() noexcept(false) { flag_ = true; }
            bool& flag_;
        };

        test t {destructor_called};
        REQUIRE(!noexcept(call_destructor(t)));
        call_destructor(t);
        REQUIRE(destructor_called);
    }
}

TEST_CASE("sub_region_iterator") {
    using namespace boken;

    constexpr int w = 5;
    constexpr int h = 4;

    std::vector<int> const v {
        0,   1,  2,  3,  4
      , 10, 11, 12, 13, 14
      , 20, 21, 22, 23, 24
      , 30, 31, 32, 33, 34
    };

    REQUIRE(v.size() == static_cast<size_t>(w * h));

    SECTION("fully contained sub region") {
        constexpr int offx = 1;
        constexpr int offy = 1;
        constexpr int sw   = 3;
        constexpr int sh   = 2;

        auto const p = make_sub_region_range(v.data(), offx, offy, w, h, sw, sh);
        auto it      = p.first;
        auto last    = p.second;

        REQUIRE((last - it) == 6);

        std::array<int, 6> const expected {
            11, 12, 13
          , 21, 22, 23
        };

        std::vector<int> actual;
        std::copy(it, last, back_inserter(actual));

        REQUIRE(std::equal(begin(expected), end(expected)
                         , begin(actual),   end(actual)));

    }

    SECTION("rebound fully contained sub region") {
        constexpr int offx = 1;
        constexpr int offy = 1;
        constexpr int sw   = 2;
        constexpr int sh   = 2;

        std::vector<char> const v0 {
            'a', 'a', 'a', 'a', 'a'
          , 'a', 'B', 'C', 'a', 'a'
          , 'a', 'D', 'E', 'a', 'a'
          , 'a', 'a', 'a', 'a', 'a'
        };

        auto const p = make_sub_region_range(v.data(), offx, offy, w, h, sw, sh);
        auto it      = const_sub_region_iterator<char> {p.first,  v0.data()};
        auto last    = const_sub_region_iterator<char> {p.second, v0.data()};

        REQUIRE((last - it) == 4);

        std::array<int, 4> const expected {
            'B', 'C'
          , 'D', 'E'
        };

        std::vector<char> actual;
        std::copy(it, last, back_inserter(actual));
        REQUIRE(std::equal(begin(expected), end(expected)
                         , begin(actual),   end(actual)));
    }
}

#endif // !defined(BK_NO_TESTS)
