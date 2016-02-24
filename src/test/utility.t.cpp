#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "utility.hpp"

#include <bkassert/assert.hpp>

#include <array>
#include <algorithm>
#include <vector>
#include <iterator>
#include <cstddef>

namespace boken {

template <typename T>
class sub_region_iterator : public std::iterator_traits<T*> {
    using this_t = sub_region_iterator<T>;
public:
    using reference = typename std::iterator_traits<T*>::reference;
    using pointer   = typename std::iterator_traits<T*>::pointer;

    sub_region_iterator(
        T* const p
      , ptrdiff_t const off_x,       ptrdiff_t const off_y
      , ptrdiff_t const width_outer, ptrdiff_t const height_outer
      , ptrdiff_t const width_inner, ptrdiff_t const height_inner
    ) noexcept
      : p_ {p + off_x + off_y * width_outer}
      , off_x_ {off_x}
      , off_y_ {off_y}
      , width_outer_ {width_outer}
      , width_inner_ {width_inner}
      , height_inner_ {height_inner}
    {
        BK_ASSERT(!!p);
        BK_ASSERT(off_x >= 0 && off_y >= 0);
        BK_ASSERT(width_inner >= 0 && width_outer >= width_inner + off_x);
        BK_ASSERT(height_inner >= 0 && height_outer >= height_inner + off_y);
    }

    reference operator*() const noexcept {
        return *p_;
    }

    pointer operator->() const noexcept {
        return &**this;
    }

    void operator++() noexcept {
        ++p_;
        if (++x_ < width_inner_) {
            return;
        }

        if (++y_ < height_inner_) {
            x_ = 0;
            p_ += (width_outer_ - width_inner_);
        }
    }

    sub_region_iterator operator++(int) noexcept {
        auto result = *this;
        ++(*this);
        return result;
    }

    bool operator<(this_t const& other) const noexcept {
        return p_ < other.p_;
    }

    bool operator==(this_t const& other) const noexcept {
        return (p_ == other.p_);
    }

    bool operator!=(this_t const& other) const noexcept {
        return !(*this == other);
    }
private:
    T* p_ {};

    ptrdiff_t off_x_ {};
    ptrdiff_t off_y_ {};
    ptrdiff_t width_outer_ {};
    ptrdiff_t width_inner_ {};
    ptrdiff_t height_inner_ {};

    ptrdiff_t x_ {};
    ptrdiff_t y_ {};
};


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

    constexpr int offx = 1;
    constexpr int offy = 1;
    constexpr int sw   = 3;
    constexpr int sh   = 2;

    sub_region_iterator<int const> it  {v.data(), offx, offy, w, h, sw, sh};
    sub_region_iterator<int const> last {
        v.data() + (offx + sw) + (offy + sh - 1) * w, 0, 0, w, h, 0, 0};

    std::array<int, 6> const expected {
        11, 12, 13
      , 21, 22, 23
    };

    std::vector<int> actual;
    std::copy(it, last, back_inserter(actual));
    REQUIRE(std::equal(begin(expected), end(expected), begin(actual), end(actual)));
}

#endif // !defined(BK_NO_TESTS)
