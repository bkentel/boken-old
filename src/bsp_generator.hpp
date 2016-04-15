#pragma once

#include "math.hpp"          // for axis_aligned_rect
#include "types.hpp"         // for sizei, sizeix, sizeiy
#include "utility.hpp"
#include <memory>            // for unique_ptr
#include <vector>            // for vector, vector<>::const_iterator
#include <cstdint>           // for uint16_t

namespace boken { class random_state; }

namespace boken {

//! @note final nodes are sorted first, in descending order, by
//! min(width, height) and then by area.
class bsp_generator {
public:
    struct param_t {
        static constexpr int32_t default_width           {100};
        static constexpr int32_t default_height          {100};
        static constexpr int32_t default_min_region_size {3};
        static constexpr int32_t default_max_region_size {20};
        static constexpr int32_t default_min_room_size   {3};
        static constexpr int32_t default_max_room_size   {20};
        static constexpr int32_t default_room_chance_num {60};
        static constexpr int32_t default_room_chance_den {100};

        sizei32x width           {default_width};
        sizei32y height          {default_height};
        sizei32  min_region_size {default_min_region_size};
        sizei32  max_region_size {default_max_region_size};
        sizei32  min_room_size   {default_min_room_size};
        sizei32  max_room_size   {default_max_room_size};
        sizei32  room_chance_num {default_room_chance_num};
        sizei32  room_chance_den {default_room_chance_den};

        weight_list<int32_t, int32_t> weights {};

        float split_variance = 5.0f;
    };

    struct node_t {
        recti32  rect;
        uint16_t parent;
        uint16_t child;
        uint16_t level;
    };

    using iterator = std::vector<node_t>::const_iterator;

    virtual ~bsp_generator();

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
