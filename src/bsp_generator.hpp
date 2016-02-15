#pragma once

#include "types.hpp"
#include "math.hpp"

#include <type_traits>
#include <initializer_list>
#include <algorithm>
#include <vector>
#include <utility>

namespace boken {

class random_state;

template <typename T>
inline constexpr auto enum_value(T const e) noexcept {
    static_assert(std::is_enum<T>::value, "");
    return static_cast<std::underlying_type_t<T>>(e);
}

template <typename Key, typename Value = Key>
struct weight_list {
    using key_type   = Key;
    using value_type = Value;
    using pair_type  = std::pair<key_type, value_type>;

    weight_list(std::initializer_list<pair_type> const il) {
        *this = il;
    }

    weight_list& operator=(std::initializer_list<pair_type> const il) {
        weights.assign(il);
        std::sort(begin(weights), end(weights)
          , [](auto const a, auto const b) noexcept {
                return b.first < a.first;
            });
        return *this;
    }

    inline Value operator[](Key const k) const noexcept {
        for (auto const p : weights) {
            if (p.first < k) { return p.second; }
        }
        return Value {};
    }

    std::vector<pair_type> weights;
};

//!
//!
//! @note final nodes are sorted first, in descending order, by
//! min(width, height) and then by area.
class bsp_generator {
public:
    using rect_t  = axis_aligned_rect<int>;

    struct param_t {
        static constexpr int default_width           {100};
        static constexpr int default_height          {100};
        static constexpr int default_min_region_size {3};
        static constexpr int default_max_region_size {20};
        static constexpr int default_min_room_size   {3};
        static constexpr int default_max_room_size   {20};
        static constexpr int default_room_chance_num {60};
        static constexpr int default_room_chance_den {100};

        static constexpr int default_max_weight {1000};

        sizeix width           {default_width};
        sizeiy height          {default_height};
        sizei  min_region_size {default_min_region_size};
        sizei  max_region_size {default_max_region_size};
        sizei  min_room_size   {default_min_room_size};
        sizei  max_room_size   {default_max_room_size};
        sizei  room_chance_num {default_room_chance_num};
        sizei  room_chance_den {default_room_chance_den};

        int              max_weight {default_max_weight};
        weight_list<int> weights    {{0, default_max_weight}};

        float max_aspect     = 16.0f / 10.0f;
        float split_variance = 5.0f;
    };

    struct node_t {
        rect_t   rect;
        uint16_t parent;
        uint16_t child;
        uint16_t level;
    };

    using iterator = std::vector<node_t>::const_iterator;

    virtual ~bsp_generator() = default;

    virtual param_t& params() noexcept = 0;

    param_t const& params() const noexcept {
        return const_cast<bsp_generator*>(this)->params();
    }

    virtual void generate(random_state& rng) = 0;

    virtual size_t size()  const noexcept = 0;
    virtual bool   empty() const noexcept = 0;

    virtual iterator begin() const noexcept = 0;
    virtual iterator end()   const noexcept = 0;

    virtual void clear() noexcept = 0;

    node_t operator[](size_t const i) const noexcept {
        return at_(i);
    }
private:
    virtual node_t at_(size_t i) const noexcept = 0;
};

std::unique_ptr<bsp_generator> make_bsp_generator(bsp_generator::param_t p);

} // namespace boken