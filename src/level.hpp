#pragma once

#include "types.hpp"
#include "math.hpp"
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
enum class tile_id : uint32_t;

struct tile_view {
    tile_id    const&       id;
    tile_type  const&       type;
    tile_flags const&       flags;
    uint16_t   const&       tile_index;
    uint16_t   const&       region_id;
    tile_data  const* const data;
};

enum class placement_result : unsigned {
    ok, failed_obstacle, failed_entity, failed_bounds, failed_bad_id
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

    virtual item   const* find(item_instance_id   id) const noexcept = 0;
    virtual entity const* find(entity_instance_id id) const noexcept = 0;

    virtual entity const* entity_at(point2i p) const noexcept = 0;
    virtual item   const* item_at(point2i p)   const noexcept = 0;

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
    virtual placement_result add_item_at(item&& i, point2i p) = 0;
    virtual placement_result add_entity_at(entity&& e, point2i p) = 0;

    virtual std::pair<point2i, placement_result>
        add_item_nearest_random(random_state& rng, item&& i, point2i p, int max_distance) = 0;
    virtual std::pair<point2i, placement_result>
        add_entity_nearest_random(random_state& rng, entity&& e, point2i p, int max_distance) = 0;

    virtual placement_result move_by(item_instance_id   id, vec2i v) noexcept = 0;
    virtual placement_result move_by(entity_instance_id id, vec2i v) noexcept = 0;

    //===--------------------------------------------------------------------===
    //                         Block-based data access
    //===--------------------------------------------------------------------===
    virtual std::vector<point2<uint16_t>> const& entity_positions() const noexcept = 0;
    virtual std::vector<entity_id>        const& entity_ids()       const noexcept = 0;

    virtual std::pair<std::vector<uint16_t> const&, recti>
        tile_indicies(int block) const noexcept = 0;

    virtual std::pair<std::vector<uint16_t> const&, recti>
        region_ids(int block) const noexcept = 0;
};

std::unique_ptr<level> make_level(random_state& rng, sizeix width, sizeiy height);

} //namespace boken
