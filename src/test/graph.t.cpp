#if !defined(BK_NO_TESTS)
#include "catch.hpp"

#include "bkassert/assert.hpp"
#include <vector>

namespace boken {

template <typename VertexData>
class vertex_data {
public:
    using data_type = VertexData;

    explicit vertex_data(int const n, data_type const& data = data_type {}) {
        BK_ASSERT(n >= 0);
        vertex_data_.resize(static_cast<size_t>(n), data);
    }

    data_type const& operator()(int const vertex) const noexcept {
        return vertex_data_[index_of_(vertex)];
    }

    data_type& operator()(int const vertex) noexcept {
        return vertex_data_[index_of_(vertex)];
    }

    int size() const noexcept {
        return static_cast<int>(vertex_data_.size());
    }

    void clear(data_type const& value = data_type {}) {
        auto const n = vertex_data_.size();
        vertex_data_.clear();
        vertex_data_.resize(n, value);
    }
private:
    size_t index_of_(int const vertex) const noexcept {
        BK_ASSERT(vertex >= 0
               && vertex < size());
        return static_cast<size_t>(vertex);
    }

    std::vector<data_type> vertex_data_;
};

template <typename EdgeType>
class adjacency_matrix {
    static size_t check_size_(int const n) noexcept {
        BK_ASSERT(n >= 0);
        return static_cast<size_t>(n);
    }
public:
    static_assert(std::is_integral<EdgeType>::value, "");
    using edge_type = EdgeType;

    adjacency_matrix(int const verticies)
      : size_ {check_size_(verticies)}
    {
        data_.resize(size_ * size_);
    }

    int verticies() const noexcept {
        return static_cast<int>(size_);
    }

    edge_type operator()(int const v_from, int const v_to) const noexcept {
        return static_cast<edge_type>(at_(v_from, v_to));
    }

    edge_type add_edge(int const v_from, int const v_to) noexcept {
        constexpr auto max_value = std::numeric_limits<edge_type>::max();

        auto const result = (at_(v_from, v_to) < max_value)
          ? ++at_(v_from, v_to)
          : max_value;

        return static_cast<edge_type>(result);
    }

    std::pair<edge_type, edge_type> add_mutual_edge(int const v_from, int const v_to) noexcept {
        return {add_edge(v_from, v_to)
              , add_edge(v_to,   v_from)};
    }

    edge_type remove_edge(int const v_from, int const v_to) noexcept {
        auto const result = (at_(v_from, v_to) > 0)
          ? --at_(v_from, v_to)
          : 0;

        return static_cast<edge_type>(result);
    }
private:
    size_t index_of_(int const from, int const to) const noexcept {
        BK_ASSERT(from >= 0
               && to   >= 0
               && static_cast<size_t>(from) < size_
               && static_cast<size_t>(to)   < size_);
        return static_cast<size_t>(from) + static_cast<size_t>(to) * size_;
    }

    int32_t at_(int const from, int const to) const noexcept {
        return data_[index_of_(from, to)];
    }

    int32_t& at_(int const from, int const to) noexcept {
        return data_[index_of_(from, to)];
    }
private:
    //! the number of verticies in the graph
    size_t const size_;

    std::vector<int32_t> data_;
};

namespace detail {

template <typename EdgeType, typename VertexData, typename Index>
VertexData connected_components_impl(
    adjacency_matrix<EdgeType> const& graph
  , vertex_data<VertexData>&          v_data
  , std::vector<Index>&               next_list
) {
    constexpr auto unvisited = VertexData {};
    auto const     n_vertex  = static_cast<Index>(graph.verticies());

    // initialize a vector filled with each vertex in the graph
    {
        auto const n = static_cast<size_t>(n_vertex);
        next_list.clear();
        next_list.reserve(n * 2u);
        next_list.resize(n);
        std::iota(begin(next_list), end(next_list), 0);
    }

    // pop a vertex off the back of next_list
    auto const pop_next = [&] {
        auto const v = next_list.back();
        next_list.pop_back();
        return v;
    };

    // pop all visited verticies off of next_list
    auto const discard_visited = [&]() noexcept {
        while (!next_list.empty() && v_data(next_list.back()) != unvisited) {
            next_list.pop_back();
        }
    };

    // for each vertex adjacent to v that hasn't been visited already, push it
    // on to next_list; return the number of such verticies
    auto const push_neighbors = [&](Index const v, VertexData const c) {
        Index n = 0;

        v_data(v) = c;
        for (Index i = 0; i < n_vertex; ++i) {
            if (v == i || !graph(v, i) || v_data(i) != unvisited) {
                continue;
            }

            next_list.push_back(i);
            v_data(i) = c;
            ++n;
        }

        return n;
    };

    v_data.clear();
    auto component = static_cast<VertexData>(unvisited + 1);

    while (!next_list.empty()) {
        for (auto v = pop_next()
           ; push_neighbors(v, component) > 0
           ; v = pop_next()) {
        }

        ++component;
        discard_visited();
    }

    return --component;
}

} // namespace detail

template <typename EdgeType, typename VertexData>
VertexData connected_components(
    adjacency_matrix<EdgeType> const& graph
  , vertex_data<VertexData>&          v_data
) {
    std::vector<int16_t> buffer;
    return detail::connected_components_impl(graph, v_data, buffer);
}

template <typename EdgeType, typename VertexData, typename Callback>
VertexData connect_components(
    adjacency_matrix<EdgeType> const& graph
  , vertex_data<VertexData>&          v_data
  , Callback                          on_unconnected
) {
    std::vector<int16_t> buffer;

    while (detail::connected_components_impl(graph, v_data, buffer) >= 2) {
        on_unconnected();
    }
}

} // namespace boken

TEST_CASE("graph connected_components") {
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

    return;
}

#endif // !defined(BK_NO_TESTS)
