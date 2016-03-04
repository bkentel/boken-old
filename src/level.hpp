#pragma once

#include "math.hpp"   // for point2i, recti, vec2i, axis_aligned_rect, etc
#include "types.hpp"  // for entity_instance_id, item_instance_id, sizeix, etc
#include "utility.hpp"

#include <memory>     // for unique_ptr
#include <utility>    // for pair
#include <vector>     // for vector
#include <cstdint>    // for uint16_t, int32_t
#include <functional>

namespace boken { class entity; }
namespace boken { class item; }
namespace boken { class item_pile; }
namespace boken { class random_state; }
namespace boken { class tile_flags; }
namespace boken { struct tile_data; }
namespace boken { struct tile_data_set; }
namespace boken { class world; }
namespace boken { enum class tile_type : uint16_t; }
namespace boken { enum class tile_id : uint32_t; }

namespace boken {

struct tile_view {
    tile_id    const&       id;
    tile_type  const&       type;
    tile_flags const&       flags;
    uint16_t   const&       region_id;
    tile_data  const* const data;
};

enum class placement_result : uint32_t {
    ok, failed_obstacle, failed_entity, failed_bounds, failed_bad_id
};

enum class move_item_result : uint32_t {
    ok, failed_full, failed_over_weight
};

struct region_info {
    recti   bounds;
    int32_t entity_count;
    int32_t item_count;
    int32_t tile_count;
};

//====---
// A generic level concept
//====---
class level {
public:
    virtual ~level();

    //===--------------------------------------------------------------------===
    //                               Queries
    //===--------------------------------------------------------------------===
    virtual sizeix width()  const noexcept = 0;
    virtual sizeiy height() const noexcept = 0;
    virtual recti  bounds() const noexcept = 0;

    virtual item   const* find(item_instance_id   id) const noexcept = 0;

    virtual std::pair<entity const*, point2i> find(entity_instance_id id) const noexcept = 0;
    virtual std::pair<entity*, point2i> find(entity_instance_id id) noexcept = 0;

    virtual entity    const* entity_at(point2i p) const noexcept = 0;
    virtual item_pile const* item_at(point2i p)   const noexcept = 0;

    bool is_entity_at(point2i const p) const noexcept {
        return !!entity_at(p);
    }

    bool is_item_at(point2i const p) const noexcept {
        return !!item_at(p);
    }

    virtual placement_result can_place_entity_at(point2i p) const noexcept = 0;
    virtual placement_result can_place_item_at(point2i p) const noexcept = 0;

    virtual size_t region_count() const noexcept = 0;
    virtual region_info region(size_t i) const noexcept = 0;

    virtual tile_view at(point2i) const noexcept = 0;
    tile_view at(int x, int y) const noexcept {
        return at(point2i {x, y});
    }

    //===--------------------------------------------------------------------===
    //                          State Mutation
    //===--------------------------------------------------------------------===
    virtual void transform_entities(std::function<point2i (entity&, point2i)>&& f) = 0;

    virtual placement_result add_item_at(unique_item i, point2i p) = 0;
    virtual placement_result add_entity_at(entity&& e, point2i p) = 0;

    virtual std::pair<point2i, placement_result>
        add_item_nearest_random(random_state& rng, item&& i, point2i p, int max_distance) = 0;
    virtual std::pair<point2i, placement_result>
        add_entity_nearest_random(random_state& rng, entity&& e, point2i p, int max_distance) = 0;

    virtual placement_result move_by(item_instance_id   id, vec2i v) noexcept = 0;
    virtual placement_result move_by(entity_instance_id id, vec2i v) noexcept = 0;

    virtual const_sub_region_range<tile_id>
        update_tile_at(random_state& rng, point2i p, tile_data_set const& data) noexcept = 0;

    using move_item_callback = std::function<void (move_item_result, item const&)>;
    virtual int move_items(point2i from, entity& to, move_item_callback const& on_fail) = 0;
    virtual int move_items(point2i from, item& to, move_item_callback const& on_fail) = 0;
    virtual int move_items(point2i from, item_pile& to, move_item_callback const& on_fail) = 0;

    //===--------------------------------------------------------------------===
    //                         Block-based data access
    //===--------------------------------------------------------------------===
    virtual std::vector<point2<uint16_t>> const& entity_positions() const noexcept = 0;
    virtual std::vector<entity_id>        const& entity_ids()       const noexcept = 0;

    virtual std::vector<point2<uint16_t>> const& item_positions() const noexcept = 0;
    virtual std::vector<item_id>          const& item_ids()       const noexcept = 0;

    virtual const_sub_region_range<tile_id> tile_ids(recti area) const noexcept = 0;

    virtual const_sub_region_range<uint16_t> region_ids(recti area) const noexcept = 0;
};

std::unique_ptr<level> make_level(random_state& rng, world& w, sizeix width, sizeiy height);

} //namespace boken
