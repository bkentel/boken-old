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

struct tile_view {
    tile_id    const&       id;
    tile_type  const&       type;
    tile_flags const&       flags;
    uint16_t   const&       tile_index;
    uint16_t   const&       region_id;
    tile_data  const* const data;
};

//====---
// A generic level concept
//====---
class level {
public:
    virtual ~level() = default;

    virtual item   const* find(item_instance_id   id) const noexcept = 0;
    virtual entity const* find(entity_instance_id id) const noexcept = 0;

    virtual int region_count() const noexcept = 0;

    virtual tile_view at(int x, int y) const noexcept = 0;

    virtual std::pair<std::vector<uint16_t> const&, recti>
        tile_indicies(int block) const noexcept = 0;

    virtual std::pair<std::vector<uint16_t> const&, recti>
        region_ids(int block) const noexcept = 0;
};

std::unique_ptr<level> make_level(random_state& rng, sizeix width, sizeiy height);

} //namespace boken
