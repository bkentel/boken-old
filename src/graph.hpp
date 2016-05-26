#pragma once

#include "math_types.hpp"

#include "bkassert/assert.hpp"

#include <vector>
#include <queue>
#include <type_traits>
#include <limits>
#include <iterator>
#include <tuple>

#include <cstdint>
#include <cstddef>

namespace boken {

//! vertex specific data for use with adjacency_matrix
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

//! An adjacency matrix representation of an (un)directed graph.
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

//! Get the number of connected components in @p graph. The 1-based component
//! each vertex in the graph belongs to is written to @p v_data.
template <typename EdgeType, typename VertexData>
VertexData connected_components(
    adjacency_matrix<EdgeType> const& graph
  , vertex_data<VertexData>&          v_data
) {
    std::vector<int16_t> buffer;
    return detail::connected_components_impl(graph, v_data, buffer);
}

//! As long as there is more than one connected component in @p graph, invoke
//! the supplied callback @p on_unconnected with the number of components in the
//! graph. Control returns to the caller when the graph is fully connected.
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
auto count_components(vertex_data<T> const& data, Container& out, size_t const n) {
    using iterator = typename std::decay_t<Container>::iterator;
    using tag = typename std::iterator_traits<iterator>::iterator_category;
    static_assert(std::is_same<tag, std::random_access_iterator_tag>::value, "");

    out.clear();
    out.resize(n);

    for (T const c : data) {
        BK_ASSERT(c > 0 && static_cast<size_t>(c - 1) < n);
        ++out[static_cast<size_t>(c - 1)]; // components are 1-based
    }

    size_t min_i = 0;
    size_t max_i = 0;

    for (size_t i = 0; i < n; ++i) {
        auto const count = out[i];
        if (count < out[min_i]) {
            min_i = i;
        } else if (count > out[max_i]) {
            max_i = i;
        }
    }

    return std::make_tuple(min_i, max_i, out[min_i], out[max_i]);
}

//! Graph must support the following interface:
//! Graph {
//!   using point = <point type>
//!   bool is_passable(point) const;
//!   bool is_in_bounds(point) const;
//!   int32_t cost(point from, point to) const;
//!   int32_t width() const;
//!   int32_t height() const;
//!   int32_t size() const;
//!   void for_each_neighbor_if(point, Predicate, UnaryF) const;
//! }
template <typename Graph>
struct a_star_pather {
    using Point = typename Graph::point;
    using Width = decltype(std::declval<Graph>().width());

    //! @returns goal if a path exits from start to goal; otherwise returns the
    //!          best point that is reachable with respect to the heuristic.
    //! @param h A binary function of the form f(point p, point goal) -> int
    template <typename Heuristic>
    Point search(
        Graph const& graph
      , Point const  start
      , Point const  goal
      , Heuristic h
    ) {
        w_ = graph.width();
        clear();
        data_.resize(static_cast<size_t>(graph.size()));

        auto& frontier = pqueue_;

        // keep track of the 'best' node with respect to the heuristic
        int32_t min_h   = std::numeric_limits<int32_t>::max();
        Point   closest = start;

        frontier.push({start, 0});
        visit(start, start, 0);

        while (!frontier.empty()) {
            auto const current = frontier.top().first;
            frontier.pop();

            if (current == goal) {
                closest = goal;
                break;
            }

            auto const current_cost = cost_so_far(current).first;
            int32_t new_cost = 0;

            graph.for_each_neighbor_if(current
              , [&](Point const next) noexcept {
                    new_cost = current_cost + graph.cost(current, next);
                    auto const cost = cost_so_far(next);

                    // the node hasn't been visited, or the new cost to the node
                    // is lower than the existing one
                    return !cost.second || new_cost < cost.first;
                }
              , [&](Point const next) noexcept {
                    visit(next, current, new_cost);

                    // update the best node
                    // TODO: consider using the cross product to break ties
                    auto const h_value = h(next, goal);
                    if (h_value < min_h) {
                        min_h = h_value;
                        closest = next;
                    }

                    frontier.push({next, new_cost + h_value});
                });
        }

        return closest;
    }

    template <typename OutputIt>
    void reverse_copy_path(
        Point    const start
      , Point    const goal
      , OutputIt       it
    ) const noexcept {
        // no path to goal
        if (data_[index_of(goal)] == 0) {
            return;
        }

        for (auto p = goal; p != start; ++it) {
            *it = p;
            p = came_from(p);
        }

        *it = start;
    }
private:
    void clear() {
        // nasty hack around the lack of a clear() function for queues.
        // this uses a trick to "subvert" the access to the protected c member.
        using queue_t = std::decay_t<decltype(pqueue_)>;
        struct clear_t : queue_t {
            static void clear(queue_t& q) noexcept {
                (q.*&clear_t::c).clear();
            }
        };

        clear_t::clear(pqueue_);
        data_.clear();
    }

    size_t index_of(Point const p) const noexcept {
        return static_cast<size_t>(value_cast(p.x) + value_cast(p.y) * w_);
    }

    static uint32_t encode_dir(Point const p, Point const from) noexcept {
        auto const encode = [](auto const n) noexcept -> uint32_t {
            auto const n0 = value_cast(n);
            return (n0 <  0) ? 0b11u
                 : (n0 == 0) ? 0b01u
                 : (n0 >  0) ? 0b10u : 0b00u;
        };

        auto const v = from - p;
        return (encode(v.y) << 28) | (encode(v.x) << 30);
    }

    static vec2<int> decode_dir(uint32_t const n) noexcept {
        switch (n >> 28) {
        //   0bXXYY
        case 0b1111 : return {-1, -1};
        case 0b1101 : return {-1,  0};
        case 0b1110 : return {-1,  1};
        case 0b0111 : return { 0, -1};
        case 0b0101 : return { 0,  0};
        case 0b0110 : return { 0,  1};
        case 0b1011 : return { 1, -1};
        case 0b1001 : return { 1,  0};
        case 0b1010 : return { 1,  1};
        default     : break;
        }

        return {0, 0};
    }

    void visit(Point const p, Point const from, int32_t const cost) noexcept {
        auto const d = encode_dir(p, from);
        auto const c = static_cast<uint32_t>(cost) & ~(0b1111u << 28);
        data_[index_of(p)] = c | d;
    }

    std::pair<int32_t, bool> cost_so_far(Point const p) const noexcept {
        auto const n = data_[index_of(p)];
        return {static_cast<int32_t>(n & ~(0b1111u << 28)), !!(n >> 28)};
    }

    Point came_from(Point const p) const noexcept {
        return p + decode_dir(data_[index_of(p)]);
    }
private:
    Width w_;

    using cost_t = std::pair<Point, int32_t>;

    struct greater {
        constexpr bool operator()(cost_t const a, cost_t const b) const noexcept {
            return a.second > b.second;
        }
    };

    std::priority_queue<cost_t, std::vector<cost_t>, greater> pqueue_;

    // XX'YY'CCCC'CCCCCCCC'CCCCCCCC'CCCCCCCC
    // 28 bits for cost, 4 bits for "from" direction
    // 00 ->  unvisited
    // 01 ->  0
    // 10 ->  1
    // 11 -> -1
    std::vector<uint32_t> data_;
};

template <typename Graph>
auto make_a_star_pather(Graph const&) {
    return a_star_pather<Graph> {};
}

inline auto diagonal_heuristic() noexcept {
    return [](auto const p, auto const goal) noexcept {
        auto const v = abs(goal - p);
        return std::max(value_cast(v.x), value_cast(v.y));
    };
}

} // namespace boken
