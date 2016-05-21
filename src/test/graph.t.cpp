#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "graph.hpp"

#include "math_types.hpp"
#include "math.hpp"
#include <queue>
#include <array>

namespace boken {

template <typename T = int32_t>
class grid_graph {
public:
    using point = point2<T>;

    grid_graph(int32_t const width, int32_t const height) noexcept
      : width_  {width}
      , height_ {height}
    {
    }

    bool is_passable(point const p) const noexcept {
        if (value_cast(p.x) == 1 && value_cast(p.y) < 15) {
            return false;
        }

        return true;
    }

    bool is_in_bounds(point const p) const noexcept {
        auto const x = value_cast(p.x);
        auto const y = value_cast(p.y);

        return (x >= 0 && x < width_)
            && (y >= 0 && y < height_);
    }

    int32_t cost(
        point const //from
      , point const //to
    ) const noexcept {
        return 1;
    }

    template <typename Predicate, typename UnaryF>
    void for_each_neighbor_if(point const p, Predicate pred, UnaryF f) const noexcept {
        using v = vec2<int>;
        constexpr std::array<v, 8> dir {
            v {-1, -1}, v { 0, -1}, v { 1, -1}
          , v {-1,  0},             v { 1,  0}
          , v {-1,  1}, v { 0,  1}, v { 1,  1}
        };

        for (auto const& d : dir) {
            point const p0 = p + underlying_cast_unsafe<T>(d);
            if (is_in_bounds(p0) && pred(p0) && is_passable(p0)) {
                f(p0);
            }
        }
    }

    int32_t width()  const noexcept { return width_; }
    int32_t height() const noexcept { return height_; }
    int32_t size()   const noexcept { return width_ * height_; }
private:
    int32_t width_;
    int32_t height_;
};

} // namespace boken

TEST_CASE("a_star_pather") {
    using namespace boken;

    grid_graph<> graph {20, 20};

    auto pather = make_a_star_pather(graph);

    auto const start = point2i32 {0, 0};
    auto const goal  = point2i32 {10, 10};

    auto const p = pather.search(graph, start, goal, diagonal_heuristic());
    REQUIRE(p == goal);

    std::vector<point2i32> path;
    pather.reverse_copy_path(start, goal, back_inserter(path));
    std::reverse(begin(path), end(path));

    REQUIRE(path.size() >= 10u);
    REQUIRE(path.front() == start);
    REQUIRE(path.back() == goal);
}

TEST_CASE("graph connected_components 1") {
    using namespace boken;

    adjacency_matrix<int> graph {5};

    graph.add_mutual_edge(0, 1);
    graph.add_mutual_edge(1, 2);
    graph.add_mutual_edge(3, 4);

    vertex_data<int8_t> v_data {graph.verticies()};

    auto const components = connected_components(graph, v_data);

    REQUIRE(components == 2);

    REQUIRE(v_data(0) == v_data(1));
    REQUIRE(v_data(1) == v_data(2));

    REQUIRE(v_data(3) == v_data(4));
}

TEST_CASE("graph connected_components 2") {
    using namespace boken;

    adjacency_matrix<int> graph {10};

    graph.add_mutual_edge(0, 1);
    graph.add_mutual_edge(1, 2);
    graph.add_mutual_edge(3, 4);
    graph.add_mutual_edge(4, 5);
    graph.add_mutual_edge(5, 6);
    graph.add_mutual_edge(8, 9);

    vertex_data<int8_t> v_data {graph.verticies()};

    auto const components = connected_components(graph, v_data);

    REQUIRE(components == 4);
}

TEST_CASE("graph connect_components") {
    using namespace boken;

    adjacency_matrix<int> graph {10};

    graph.add_mutual_edge(0, 1);
    graph.add_mutual_edge(1, 2);
    graph.add_mutual_edge(3, 4);
    graph.add_mutual_edge(4, 5);
    graph.add_mutual_edge(5, 6);
    graph.add_mutual_edge(8, 9);

    vertex_data<int8_t> v_data {graph.verticies()};

    auto const first = begin(v_data);
    auto const last  = end(v_data);

    REQUIRE(connected_components(graph, v_data) == 4);

    connect_components(graph, v_data, [&](int const n) {
        if (n == 1) {
            return false;
        }

        auto const v0 = 0;
        auto const c0 = v_data(v0);

        auto const it = std::find_if(first, last
          , [c0](int8_t const c) noexcept { return c != c0; });

        REQUIRE(it != last);

        auto const v1 = static_cast<int>(std::distance(first, it));
        graph.add_mutual_edge(v0, v1);

        return true;
    });

    REQUIRE(connected_components(graph, v_data) == 1);
}

TEST_CASE("graph count_components") {
    using namespace boken;

    adjacency_matrix<int> graph {5};

    // [0] <--> [1] <--> [2] component 1 (index 0)
    // [3] <--> [4]          component 2 (index 1)

    graph.add_mutual_edge(0, 1);
    graph.add_mutual_edge(1, 2);
    graph.add_mutual_edge(3, 4);

    vertex_data<int8_t> v_data {graph.verticies()};

    auto const components = connected_components(graph, v_data);
    REQUIRE(components == 2);

    size_t  min_i = 0;
    size_t  max_i = 0;
    int32_t min_n = 0;
    int32_t max_n = 0;

    std::vector<int32_t> counts;
    std::tie(min_i, max_i, min_n, max_n) =
        count_components(v_data, counts, static_cast<size_t>(components));

    REQUIRE(min_i == 1u);
    REQUIRE(max_i == 0u);
    REQUIRE(min_n == 2);
    REQUIRE(max_n == 3);

    REQUIRE(counts.size() == 2u);
    REQUIRE(counts[0] == 3);
    REQUIRE(counts[1] == 2);
}

#endif // !defined(BK_NO_TESTS)
