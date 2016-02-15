#include "bsp_generator.hpp"
#include "random.hpp"
#include "utility.hpp"
#include <tuple>

namespace boken {

template <typename T>
T&& choose_largest(random_state& rng, T&& a, T&& b) {
    return std::forward<T>(
        a < b ? b
      : b < a ? a
      : random_coin_flip(rng) ? a : b);
}

template <typename Int, typename R = axis_aligned_rect<Int>>
std::pair<R, R> slice_rect(
    random_state&        rng
  , R              const rect
  , size_type<Int> const min_size
  , double         const variance = 4.0 // higher implies less variance
) noexcept {
    Int const  w = rect.width();
    Int const  h = rect.height();
    Int const& x = choose_largest(rng, w, h);
    Int const  p = round_as<Int>(random_normal(rng, x / 2.0, x / variance));
    Int const  n = clamp(p, value_cast(min_size), x - value_cast(min_size));

    R r0 = rect;
    R r1 = rect;

    (&x == &w) ? (r1.x0 = (r0.x1 = r0.x0 + n))
               : (r1.y0 = (r0.y1 = r0.y0 + n));

    return {r0, r1};
}

template <typename Int>
constexpr bool can_slice_rect(
    axis_aligned_rect<Int> const r
  , size_type<Int>         const min_size
) noexcept {
    return r.width()  < value_cast(min_size) * 2
        && r.height() < value_cast(min_size) * 2;
}

template <typename Int>
constexpr bool must_slice_rect(
    axis_aligned_rect<Int> const r
  , size_type<Int>         const max_size
) noexcept {
    return r.width()  > value_cast(max_size)
        || r.height() > value_cast(max_size);
}

class bsp_generator_impl : public bsp_generator {
public:
    explicit bsp_generator_impl(param_t p)
      : params_ {std::move(p)}
    {
    }

    param_t& params() noexcept final override {
        return params_;
    }

    void generate(random_state& rng) final override;

    size_t size() const noexcept final override {
        return leaf_nodes_.size();
    }

    bool empty() const noexcept final override {
        return leaf_nodes_.empty();
    }

    iterator begin() const noexcept final override {
        return std::begin(leaf_nodes_);
    }

    iterator end() const noexcept final override {
        return std::end(leaf_nodes_);
    }

    void clear() noexcept final override {
        nodes_.clear();
        leaf_nodes_.clear();
    }
private:
    node_t at_(size_t const i) const noexcept final override {
        return nodes_[i];
    }

    param_t             params_;
    std::vector<node_t> nodes_;
    std::vector<node_t> leaf_nodes_;
};

void bsp_generator_impl::generate(random_state& rnd) {
    auto const& p = params_;

    params_.weights = {
        {400, 1000}
      , {100, 800}
      , {50,  400}
      , {25,  100}
      , {0,   100}
    };

    nodes_.clear();
    leaf_nodes_.clear();

    nodes_.push_back(node_t {
        rect_t {
            offset_type_x<int> {0}
          , offset_type_y<int> {0}
          , p.width
          , p.height}
      , 0
      , 0
    });

    auto const pass_split_chance = [&](rect_t const& r) {
        return p.weights[r.area()] >= random_uniform_int(rnd, 0, p.max_weight);
    };

    auto const add_children = [&](auto const& pair, uint16_t const i) {
        nodes_.push_back(node_t {pair.first,  i, 0});
        nodes_.push_back(node_t {pair.second, i, 0});
    };

    for (uint16_t i = 0; i != static_cast<uint16_t>(nodes_.size()); ++i) {
        node_t&     n = nodes_[i];
        auto const& r = n.rect;

        if (must_slice_rect(r, p.max_region_size)
         || can_slice_rect(r, p.min_region_size) && pass_split_chance(r)
        ) {
            n.child = i + 1;
            add_children(slice_rect(rnd, r, p.min_region_size, p.split_variance), i);
        } else {
            leaf_nodes_.push_back(n);
        }
    }

    using namespace container_algorithms;
    sort(leaf_nodes_, [](node_t const& a, node_t const& b) noexcept {
        auto const am = std::min(a.rect.width(), a.rect.height());
        auto const bm = std::min(b.rect.width(), b.rect.height());

        return (am == bm)
          ? b.rect.area() < a.rect.area()
          : bm < am;
    });
}

std::unique_ptr<bsp_generator> make_bsp_generator(bsp_generator::param_t p) {
    return std::make_unique<bsp_generator_impl>(p);
}

} //namespace boken
