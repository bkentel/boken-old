#include "bsp_generator.hpp"
#include "random.hpp"   // for random_state (ptr only), random_coin_flip, etc
#include "types.hpp"    // for value_cast, size_type, offix, offiy
#include "utility.hpp"  // for sort
#include "rect.hpp"

#include <vector>
#include <cstddef>

namespace boken {

bsp_generator::~bsp_generator() = default;

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

    const_iterator begin() const noexcept final override {
        return leaf_nodes_.data();
    }

    const_iterator end() const noexcept final override {
        return leaf_nodes_.data() + static_cast<ptrdiff_t>(leaf_nodes_.size());
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

void bsp_generator_impl::generate(random_state& rng) {
    auto const& p = params_;

    nodes_.clear();
    leaf_nodes_.clear();

    nodes_.push_back({
        {point2i32 {}, p.width, p.height}, 0, 0, 0});

    auto const pass_split_chance = [&](recti32 const r) noexcept {
        auto const area = value_cast(r.area());

        if (area >= 400) {
            return true;
        } else if (area >= 100) {
            return random_chance_in_x(rng, 1, 2);
        } else if (area >= 50) {
            return random_chance_in_x(rng, 1, 4);
        } else if (area >= 25) {
            return random_chance_in_x(rng, 1, 8);
        }

        return random_chance_in_x(rng, 1, 16);
    };

    auto const min_w = sizei32x {value_cast(p.min_region_size)};
    auto const min_h = sizei32y {value_cast(p.min_region_size)};

    auto const max_w = sizei32x {value_cast(p.max_region_size)};
    auto const max_h = sizei32y {value_cast(p.max_region_size)};

    auto const split_variance = static_cast<double>(p.split_variance);

    for (size_t i = 0; i != nodes_.size(); ++i) {
        node_t&     n = nodes_[i];
        auto const& r = n.rect;

        // neither the need nor roll to split
        if (!must_slice_rect(r, max_w, max_h) && !pass_split_chance(r)) {
            leaf_nodes_.push_back(n);
            continue;
        }

        auto const child_rects =
            slice_rect(rng, r, min_w, min_h, split_variance);

        // couldn't split
        if (child_rects.first == r) {
            leaf_nodes_.push_back(n);
            continue;
        }

        // ok
        auto const parent = static_cast<uint16_t>(i);

        nodes_.push_back({child_rects.first,  parent, 0, 0});
        nodes_.push_back({child_rects.second, parent, 0, 0});

        n.child = static_cast<uint16_t>(i + 1);
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
