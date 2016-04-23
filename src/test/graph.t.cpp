#if !defined(BK_NO_TESTS)
#include "catch.hpp"
#include "graph.hpp"

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
    std::tie(min_i, max_i, min_n, max_n) = count_components(v_data, counts);

    REQUIRE(min_i == 1u);
    REQUIRE(max_i == 0u);
    REQUIRE(min_n == 2);
    REQUIRE(max_n == 3);

    REQUIRE(counts.size() == 2u);
    REQUIRE(counts[0] == 3);
    REQUIRE(counts[1] == 2);
}

#endif // !defined(BK_NO_TESTS)
