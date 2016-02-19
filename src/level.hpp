#pragma once

#include "types.hpp"
#include "math.hpp"
#include <bkassert/assert.hpp>
#include <memory>
#include <vector>
#include <utility>

namespace boken {

class entity;
class item;
class random_state;
class tile_flags;
struct tile_data;
enum class tile_type : uint16_t;

struct tile_view {
    tile_id    const&       id;
    tile_type  const&       type;
    tile_flags const&       flags;
    uint16_t   const&       tile_index;
    uint16_t   const&       region_id;
    tile_data  const* const data;
};

enum class placement_result : unsigned {
    ok, failed_obstacle, failed_entity, failed_bounds
};

enum class move_result : unsigned {
    ok, failed_obstacle, failed_entity, failed_bounds, failed_bad_id
};


enum class convertion_type {
    unchecked, clamp, fail, modulo
};

template <convertion_type T>
using convertion_t = std::integral_constant<convertion_type, T>;

template <typename T>
constexpr auto as_unsigned(T const n, convertion_t<convertion_type::clamp>) noexcept {
    return static_cast<std::make_unsigned_t<T>>(n < 0 ? 0 : n);
}

template <typename T>
constexpr auto as_unsigned(T const n, convertion_type const type = convertion_type::clamp) noexcept {
    static_assert(std::is_arithmetic<T>::value, "");
    using ct = convertion_type;
    return type == ct::clamp ? as_unsigned(n, convertion_t<ct::clamp> {})
                             : as_unsigned(n, convertion_t<ct::clamp> {});
}

//template <typename RandIt>
//class grid_view {
//public:
//    grid_view(RandIt const base, uint32_t const row_width, recti const rect) noexcept
//      : base_ {base}, row_width_ {row_width}, rect_ {rect}
//    {
//        BK_ASSERT(rect.x0 >= 0 && as_unsigned(rect.x0 + rect.width()) <= row_width);
//        BK_ASSERT(rect.y0 >= 0);
//    }
//
//    template <typename TernaryF>
//    void for_each_xy(TernaryF f) const {
//        auto const step = row_width_ - w;
//        auto       it   = base_ + (rect_.x0) + (rect_.y0 * row_width_);
//
//        for (auto y = rect_.y0; y < rect_.y1; ++y, it += step) {
//            for (auto x = rect_.x0; x < rect_.x1; ++x) {
//                f(x, y, *it++);
//            }
//        }
//    }
//
//    grid_view<RandIt> sub_view(recti const r) const {
//        BK_ASSERT(r.x0 >= 0 && rect_.x0 <= r.x0 && rect_.x1 <= r.x1);
//        BK_ASSERT(r.y0 >= 0 && rect_.y0 <= r.y0 && rect_.y1 <= r.y1);
//
//        return {base, row_width_, r};
//    }
//
//    template <typename T>
//    void fill(T const value) const noexcept {
//        auto const w    = rect_.width();
//        auto const step = row_width_ - w;
//        auto       it   = base_ + (rect_.x0) + (rect_.y0 * row_width_);
//
//        for (auto y = rect_.y0; y < rect_.y1; ++y) {
//            it = std::fill_n(it, w, value) + step;
//        }
//    }
//
//    auto&& at(uint32_t const x, uint32_t const y) const noexcept {
//        return *(base_ + (rect_.x0 + x) + ((rect_.y0 + y) * row_width_));
//    }
//private:
//    RandIt   base_;
//    uint32_t row_width_;
//    recti    rect_;
//};

//====---
// A generic level concept
//====---
class level {
public:
    virtual ~level();

    virtual sizeix width()  const noexcept = 0;
    virtual sizeiy height() const noexcept = 0;

    virtual item   const* find(item_instance_id   id) const noexcept = 0;
    virtual entity const* find(entity_instance_id id) const noexcept = 0;

    virtual placement_result add_item_at(item&& i, point2i p) = 0;
    virtual placement_result add_entity_at(entity&& e, point2i p) = 0;

    virtual move_result move_by(item_instance_id   id, vec2i v) noexcept = 0;
    virtual move_result move_by(entity_instance_id id, vec2i v) noexcept = 0;

    virtual int region_count() const noexcept = 0;

    virtual tile_view at(int x, int y) const noexcept = 0;

    virtual std::vector<point2<uint16_t>> const& entity_positions() const noexcept = 0;
    virtual std::vector<entity_id>        const& entity_ids()       const noexcept = 0;

    virtual std::pair<std::vector<uint16_t> const&, recti>
        tile_indicies(int block) const noexcept = 0;

    virtual std::pair<std::vector<uint16_t> const&, recti>
        region_ids(int block) const noexcept = 0;
};

std::unique_ptr<level> make_level(random_state& rng, sizeix width, sizeiy height);

} //namespace boken
