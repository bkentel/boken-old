#pragma once

#include "types.hpp"
#include "math_types.hpp"
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
    point2f world_to_window(point2<T> const p) const noexcept {
        return {scale_x * value_cast_unsafe<float>(p.x) + x_off
              , scale_y * value_cast_unsafe<float>(p.y) + y_off};
    }

    template <typename T>
    vec2f world_to_window(vec2<T> const v) const noexcept {
        return {scale_x * value_cast_unsafe<float>(v.x)
              , scale_y * value_cast_unsafe<float>(v.y)};
    }

    template <typename T>
    point2f window_to_world(point2<T> const p) const noexcept {
        return {(1.0f / scale_x) * value_cast_unsafe<float>(p.x) - (x_off / scale_x)
              , (1.0f / scale_y) * value_cast_unsafe<float>(p.y) - (y_off / scale_y)};
    }

    template <typename T>
    point2f window_to_world(point2<T> const p, size_type_x<T> const tile_w, size_type_y<T> const tile_h) const noexcept {
        auto const tw = value_cast_unsafe<float>(tile_w);
        auto const th = value_cast_unsafe<float>(tile_h);
        auto const q  = window_to_world(p);
        return {value_cast(q.x) / tw, value_cast(q.y) / th};
    }

    template <typename T, typename U, typename V>
    point2f world_to_window(point2<T> const p, size_type_x<U> const tile_w, size_type_y<V> const tile_h) const noexcept {
        auto const tw = value_cast_unsafe<float>(tile_w);
        auto const th = value_cast_unsafe<float>(tile_h);
        auto const q  = underlying_cast_unsafe<float>(p);
        return world_to_window(point2f {q.x * tw, q.y * th});
    }

    template <typename T>
    vec2f window_to_world(vec2<T> const v) const noexcept {
        return {(1.0f / scale_x) * value_cast_unsafe<float>(v.x)
              , (1.0f / scale_y) * value_cast_unsafe<float>(v.y)};
    }

    template <typename T>
    void center_on_world(T const wx, T const wy) const noexcept {
    }

    template <typename T>
    point2f center_window_on_world(
        point2<T> const p
      , size_type_x<T> const tile_w, size_type_y<T> const tile_h
      , size_type_x<T> const win_w,  size_type_y<T> const win_h
    ) const noexcept {
        auto const tw = value_cast(tile_w);
        auto const th = value_cast(tile_h);
        auto const ww = value_cast(win_w);
        auto const wh = value_cast(win_h);
        auto const px = value_cast(p.x);
        auto const py = value_cast(p.y);

        return {static_cast<float>((ww * 0.5) - tw * (px + 0.5))
              , static_cast<float>((wh * 0.5) - th * (py + 0.5))};
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

    virtual bool debug_toggle_show_regions() noexcept = 0;

    virtual void set_level(level const& lvl) noexcept = 0;
    virtual void set_tile_maps(std::initializer_list<std::pair<tile_map_type, tile_map const&>> tmaps) noexcept = 0;

    virtual void update_map_data() = 0;
    virtual void update_map_data(const_sub_region_range<tile_id> sub_region) = 0;

    virtual void set_tile_highlight(point2i32 p) noexcept = 0;
    virtual void clear_tile_highlight() noexcept = 0;

    template <typename T>
    struct update_t {
        point2i32 prev_pos;
        point2i32 next_pos;
        T         id;
    };

    virtual void update_data(update_t<entity_id> const* first, update_t<entity_id> const* last) = 0;
    virtual void update_data(update_t<item_id> const* first, update_t<item_id> const* last) = 0;
    virtual void clear_data() = 0;

    virtual void update_tool_tip_text(std::string text) = 0;
    virtual bool update_tool_tip_visible(bool show) noexcept = 0;
    virtual void update_tool_tip_position(point2i32 p) noexcept = 0;

    virtual void set_message_window(message_log const* window) noexcept = 0;

    virtual void set_inventory_window(inventory_list const* window) noexcept = 0;
    // TODO: consider a different method to accomplish this
    virtual void set_inventory_window_focus(bool focus) noexcept = 0;

    virtual void render(duration_t delta, view const& v) const noexcept = 0;
};

std::unique_ptr<game_renderer> make_game_renderer(system& os, text_renderer& trender);

} //namespace boken
