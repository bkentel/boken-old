#if !defined(BK_NO_TESTS)
#include "catch.hpp"

#include "spatial_map.hpp"

namespace bk = boken;

namespace { namespace test_0 {

using id_t    = uint32_t;
using point_t = bk::point2i;

struct data_t {
    id_t    id;
    point_t position;
};

id_t id_of(data_t const& d) noexcept {
    return d.id;
}

point_t position_of(data_t const& d) noexcept {
    return d.position;
}

}} // namespace {anonymous}::test_0

TEST_CASE("spatial_map") {
    using namespace test_0;

    auto const id_to_pos = [](int const i) noexcept {
        return point_t {i , i + 1};
    };

    std::vector<data_t> data;
    {
        constexpr size_t n = 10;
        data.reserve(n);

        int i = 0;
        std::generate_n(back_inserter(data), n, [&]() mutable {
            ++i;
            return data_t {static_cast<id_t>(i), id_to_pos(i)};
        });
    }

    auto smap = bk::spatial_map<id_t, point_t> {100, 80};
    smap.insert(begin(data), end(data));

    SECTION("sanity tests") {
        // size
        REQUIRE(smap.size() == data.size());

        // at
        for (int i = 0; i < static_cast<int>(data.size()); ++i) {
            id_t const* id = smap.at(id_to_pos(i + 1));
            REQUIRE(!!id);
            REQUIRE(*id == static_cast<uint32_t>(i + 1));
        }

        // near
        auto const d = bk::ceil_as<int>(std::sqrt(value_cast(
            distance2(point_t {0, 0}, data.back().position))));

        auto const n0 = smap.near(point_t {0, 0}, d, [](auto, auto) {});
        REQUIRE(static_cast<size_t>(n0) == data.size());
    }

    SECTION("update") {
        smap.update_all([](id_t, point_t const p) noexcept {
            return p + bk::vec2i {-1, 1};
        });
    }
}

#endif // !defined(BK_NO_TESTS)
