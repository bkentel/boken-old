#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "utility.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace {

void arity_0();
void arity_1(int);
void arity_2(int, int);

} //namespace

TEST_CASE("arity_of") {
    using namespace boken;

    // free functions
    static_assert(arity_of<decltype(arity_0)>::value == 0, "");
    static_assert(arity_of<decltype(arity_1)>::value == 1, "");
    static_assert(arity_of<decltype(arity_2)>::value == 2, "");

    // free function pointers
    static_assert(arity_of<decltype(&arity_0)>::value == 0, "");
    static_assert(arity_of<decltype(&arity_1)>::value == 1, "");
    static_assert(arity_of<decltype(&arity_2)>::value == 2, "");

    // lambdas convertible to free functions
    auto const lambda_0 = []() {};
    auto const lambda_1 = [](int) {};
    auto const lambda_2 = [](int, int) {};

    static_assert(arity_of<decltype(lambda_0)>::value == 0, "");
    static_assert(arity_of<decltype(lambda_1)>::value == 1, "");
    static_assert(arity_of<decltype(lambda_2)>::value == 2, "");

    // capturing lambdas
    int i {};
    auto const cap_lambda_0 = [&]() { ++i; };
    auto const cap_lambda_1 = [&](int) { ++i; };
    auto const cap_lambda_2 = [&](int, int) { ++i; };

    static_assert(arity_of<decltype(cap_lambda_0)>::value == 0, "");
    static_assert(arity_of<decltype(cap_lambda_1)>::value == 1, "");
    static_assert(arity_of<decltype(cap_lambda_2)>::value == 2, "");

    // member functions
    struct foo {
        void member_0() {}
        void member_1(int) {}
        void member_2(int, int) {}
    };

    static_assert(arity_of<decltype(&foo::member_0)>::value == 0, "");
    static_assert(arity_of<decltype(&foo::member_1)>::value == 1, "");
    static_assert(arity_of<decltype(&foo::member_2)>::value == 2, "");
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
void as_const_test(...);
} //namespace

TEST_CASE("as_const") {
    using namespace boken;

    int        a = 0;
    int const  b = 0;

    REQUIRE(a == 0); // unused variables
    REQUIRE(b == 0); // unused variables

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
