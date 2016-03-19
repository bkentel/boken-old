#pragma once

#include "math.hpp"
#include "utility.hpp"
#include <chrono>
#include <initializer_list>
#include <vector>
#include <cstdint>

namespace boken { class level; }
namespace boken { class message_log; }
namespace boken { class system; }
namespace boken { class text_renderer; }
namespace boken { class tile_map; }
namespace boken { class inventory_list; }
namespace boken { enum class tile_id : uint32_t; }
namespace boken { enum class tile_map_type : uint32_t; }

namespace boken {

//=====--------------------------------------------------------------------=====
// The current "view" into the world.
//=====--------------------------------------------------------------------=====
class view {
public:
    view() = default;

    template <typename T>
    point2f world_to_window(T const x, T const y) const noexcept {
        return {scale_x * x + x_off
              , scale_y * y + y_off};
    }

    template <typename T>
    point2f world_to_window(vec2<T> const v) const noexcept {
        return {scale_x * value_cast(v.x)
              , scale_y * value_cast(v.y)};
    }

    template <typename T>
    point2f world_to_window(offset_type_x<T> const x, offset_type_y<T> const y) const noexcept {
        return window_to_world(value_cast(x), value_cast(y));
    }

    template <typename T>
    point2f world_to_window(point2<T> const p) const noexcept {
        return window_to_world(p.x, p.y);
    }

    template <typename T>
    point2f window_to_world(T const x, T const y) const noexcept {
        return {(1.0f / scale_x) * static_cast<float>(x) - (x_off / scale_x)
              , (1.0f / scale_y) * static_cast<float>(y) - (y_off / scale_y)};
    }

    template <typename T>
    vec2f window_to_world(vec2<T> const v) const noexcept {
        return {(1.0f / scale_x) * value_cast<float>(v.x)
              , (1.0f / scale_y) * value_cast<float>(v.y)};
    }

    template <typename T>
    point2f window_to_world(offset_type_x<T> const x, offset_type_y<T> const y) const noexcept {
        return world_to_window(value_cast(x), value_cast(y));
    }

    template <typename T>
    point2f window_to_world(point2<T> const p) const noexcept {
        return world_to_window(p.x, p.y);
    }


    template <typename T>
    void center_on_world(T const wx, T const wy) const noexcept {

    }

    float x_off   = 0.0f;
    float y_off   = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
};

//=====--------------------------------------------------------------------=====
// Responsible for rendering all the various game and ui objects.
//=====--------------------------------------------------------------------=====
class game_renderer {
public:
    using clock_t     = std::chrono::high_resolution_clock;
    using timepoint_t = clock_t::time_point;
    using duration_t  = clock_t::duration;

    virtual ~game_renderer();

    virtual void set_level(level const& lvl) noexcept = 0;
    virtual void set_tile_maps(std::initializer_list<std::pair<tile_map_type, tile_map const&>> tmaps) noexcept = 0;

    virtual void update_map_data() = 0;
    virtual void update_map_data(const_sub_region_range<tile_id> sub_region) = 0;
    virtual void update_entity_data() = 0;
    virtual void update_item_data() = 0;

    template <typename T>
    struct update_t {
        point2i prev_pos;
        point2i next_pos;
        T       id;
    };

    virtual void update_entity_data(std::vector<update_t<entity_id>> const& updates) = 0;
    virtual void update_item_data(std::vector<update_t<item_id>> const& updates) = 0;

    virtual void update_tool_tip_text(std::string text) = 0;
    virtual bool update_tool_tip_visible(bool show) noexcept = 0;
    virtual void update_tool_tip_position(point2i p) noexcept = 0;

    virtual void set_message_window(message_log const* window) noexcept = 0;

    virtual void set_inventory_window(inventory_list const* window) noexcept = 0;

    virtual void render(duration_t delta, view const& v) const noexcept = 0;
};

std::unique_ptr<game_renderer> make_game_renderer(system& os, text_renderer& trender);

} //namespace boken
