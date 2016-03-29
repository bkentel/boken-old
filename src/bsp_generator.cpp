#include "bsp_generator.hpp"
#include "random.hpp"   // for random_state (ptr only), random_coin_flip, etc
#include "types.hpp"    // for value_cast, size_type, offix, offiy
#include "utility.hpp"  // for sort

namespace boken {

bsp_generator::~bsp_generator() = default;

//! @returns A reference to the largest of @p a and @p b, otherwise one of the
//! two chosen at random if equal.
template <typename T>
T&& choose_largest(random_state& rng, T&& a, T&& b) {
    return std::forward<T>(
        a < b ? b
      : b < a ? a
      : random_coin_flip(rng) ? a : b);
}

//! @return The rectangle @p rect sliced into two smaller rects along its
//! largest axis. Otherwise the input rect if the result would be smaller than
//! @p min_size.
template <typename Int
        , typename R = axis_aligned_rect<Int>>
std::pair<R, R> slice_rect(
    random_state&        rng
  , R              const rect
  , size_type<Int> const min_size
  , double         const variance = 4.0 //!< higher implies less variance
) noexcept {
    Int const  w = value_cast(rect.width());
    Int const  h = value_cast(rect.height());
    Int const& x = choose_largest(rng, w, h);
    Int const  p = round_as<Int>(random_normal(rng, x / 2.0, x / variance));
    Int const  n = clamp(p, value_cast(min_size), x - value_cast(min_size));

    R r0 = rect;
    R r1 = rect;

    if (&x == &w) {
        r1.x0 = (r0.x1 = r0.x0 + size_type_x<Int> {n});
    } else {
        r1.y0 = (r0.y1 = r0.y0 + size_type_y<Int> {n});
    }

    return {r0, r1};
}

//! @return true if @p r can be split into two smaller rects with dimensions at
//! least as big as @p min_size, otherwise false.
template <typename Int>
constexpr bool can_slice_rect(
    axis_aligned_rect<Int> const r, size_type<Int> const min_size
) noexcept {
    return value_cast(r.width())  < value_cast(min_size) * 2
        && value_cast(r.height()) < value_cast(min_size) * 2;
}

//! @return true if @p r has a dimension at least as big as @p max_size,
//! otherwise false.
template <typename Int>
constexpr bool must_slice_rect(
    axis_aligned_rect<Int> const r, size_type<Int> const max_size
) noexcept {
    return value_cast(r.width())  > value_cast(max_size)
        || value_cast(r.height()) > value_cast(max_size);
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

    nodes_.push_back({
        {point2i32 {}, p.width, p.height}, 0, 0, 0});

    auto const pass_split_chance = [&](recti32 const& r) {
        auto const area = value_cast(r.area());
        return p.weights[area] >= random_uniform_int(rnd, 0, p.max_weight);
    };

    auto const add_children = [&](auto const& pair, uint16_t const i) {
        nodes_.push_back(node_t {pair.first,  i, 0, 0});
        nodes_.push_back(node_t {pair.second, i, 0, 0});
    };

    auto const split_variance = static_cast<double>(p.split_variance);

    for (uint16_t i = 0; i != static_cast<uint16_t>(nodes_.size()); ++i) {
        node_t&     n = nodes_[i];
        auto const& r = n.rect;

        if (must_slice_rect(r, p.max_region_size)
         || (can_slice_rect(r, p.min_region_size) && pass_split_chance(r))
        ) {
            n.child = static_cast<uint16_t>(i + 1);
            add_children(slice_rect(rnd, r, p.min_region_size, split_variance), i);
        } else {
            leaf_nodes_.push_back(n);
        }
    }

    using namespace container_algorithms;
    sort(leaf_nodes_, [](node_t const& a, node_t const& b) noexcept {
        return rect_by_min_dimension(b.rect, a.rect);
    });
}

std::unique_ptr<bsp_generator> make_bsp_generator(bsp_generator::param_t p) {
    return std::make_unique<bsp_generator_impl>(p);
}

} //namespace boken
