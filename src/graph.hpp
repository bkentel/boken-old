#pragma once

#include "bkassert/assert.hpp"

#include <vector>
#include <type_traits>
#include <limits>
#include <iterator>

#include <cstdint>
#include <cstddef>

namespace boken {

//==============================================================================
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

    auto begin() const noexcept { return vertex_data_.begin(); }
    auto end()   const noexcept { return vertex_data_.end(); }
private:
    size_t index_of_(int const vertex) const noexcept {
        BK_ASSERT(vertex >= 0
               && vertex < size());
        return static_cast<size_t>(vertex);
    }

    std::vector<data_type> vertex_data_;
};

template <typename T>
auto begin(vertex_data<T> const& v) noexcept { return v.begin(); }

template <typename T>
auto end(vertex_data<T> const& v) noexcept { return v.end(); }

//==============================================================================
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

    edge_type const* begin_edges(int const vertex) const noexcept {
        return std::addressof(at_(vertex, 0));
    }

    edge_type const* end_edges(int const vertex) const noexcept {
        return std::addressof(at_(vertex, 0)) + static_cast<ptrdiff_t>(size_);
    }
private:
    size_t index_of_(int const from, int const to) const noexcept {
        BK_ASSERT(from >= 0
               && to   >= 0
               && static_cast<size_t>(from) < size_
               && static_cast<size_t>(to)   < size_);
        return static_cast<size_t>(from) * size_ + static_cast<size_t>(to);
    }

    edge_type const& at_(int const from, int const to) const noexcept {
        return data_[index_of_(from, to)];
    }

    edge_type& at_(int const from, int const to) noexcept {
        return data_[index_of_(from, to)];
    }
private:
    //! the number of verticies in the graph
    size_t const size_;

    std::vector<edge_type> data_;
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

    next_list.clear();
    next_list.reserve(static_cast<size_t>(n_vertex));

    // pop a vertex off the back of next_list
    auto const pop_next = [&] {
        auto const v = next_list.back();
        next_list.pop_back();
        return v;
    };

    // push each vertex adjacent to v that hasn't been visited already
    auto const push_neighbors = [&](Index const v0) {
        for (auto i = Index {0}; i < n_vertex; ++i) {
            if ((v0 != i) && graph(v0, i) && (v_data(i) == unvisited)) {
                next_list.push_back(i);
            }
        }
    };

    v_data.clear();
    auto component = static_cast<VertexData>(unvisited + 1);

    for (auto i = Index {0}; i < n_vertex; ++i) {
        if (v_data(i) != unvisited) {
            continue;
        }

        next_list.push_back(i);

        while (!next_list.empty()) {
            auto const v = pop_next();
            v_data(v) = component;
            push_neighbors(v);
        }

        ++component;
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
void connect_components(
    adjacency_matrix<EdgeType> const& graph
  , vertex_data<VertexData>&          v_data
  , Callback                          on_unconnected
) {
    std::vector<int16_t> buffer;

    for (;;) {
        auto const n = detail::connected_components_impl(graph, v_data, buffer);
        if (n <= 1) {
            break;
        }

        if (!on_unconnected(n)) {
            break;
        }
    }
}

//! Clears and then fills @p out with the size of each component in the graph.
//! @returns a tuple {min vertex, max vertex, min count, max count}
template <typename T, typename Container>
auto count_components(vertex_data<T> const& data, Container& out) {
    using iterator = typename std::decay_t<Container>::iterator;
    using tag = typename std::iterator_traits<iterator>::iterator_category;
    static_assert(std::is_same<tag, std::random_access_iterator_tag>::value, "");

    out.clear();

    size_t min_i = 0;
    size_t max_i = 0;

    for (T const c : data) {
        BK_ASSERT(c > 0); // components are 1-based
        auto const i = static_cast<size_t>(c - 1);

        while (out.size() <= i) {
            out.push_back({0});
        }

        auto const n = ++out[i];

        if (n < out[min_i]) {
            min_i = i;
        } else if (n > out[max_i]) {
            max_i = i;
        }
    }

    return std::make_tuple(min_i, max_i, out[min_i], out[max_i]);
}

} // namespace boken
