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

struct read_only_pointer_t {
    read_only_pointer_t() noexcept = default;

    template <typename T>
    read_only_pointer_t(
        T const* const beg
      , T const* const end
      , size_t   const stride = sizeof(T)
    ) noexcept
      : ptr            {static_cast<void const*>(beg)}
      , last           {static_cast<void const*>(end)}
      , element_size   {sizeof(T)}
      , element_stride {static_cast<uint16_t>(stride)}
    {
    }

    template <typename T>
    explicit read_only_pointer_t(std::vector<T> const& v, size_t const offset = 0, size_t const stride = sizeof(T)) noexcept
      : read_only_pointer_t {
            reinterpret_cast<T const*>(
                reinterpret_cast<char const*>(
                    reinterpret_cast<void const*>(v.data())
                ) + offset)
          , v.data() + v.size()
          , stride}
    {
    }

    read_only_pointer_t& operator++() noexcept {
        ptr = (ptr == last) ? ptr
          : reinterpret_cast<char const*>(ptr) + element_stride;
        return *this;
    }

    read_only_pointer_t operator++(int) noexcept {
        auto result = *this;
        ++(*this);
        return result;
    }

    template <typename T>
    T const& value() const noexcept {
        return *reinterpret_cast<T const*>(ptr);
    }

    explicit operator bool() const noexcept {
        return !!ptr;
    }

    void const* ptr            {};
    void const* last           {};
    uint16_t    element_size   {};
    uint16_t    element_stride {};
};

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
class renderer2d {
public:
    struct tile_params_uniform {
        sizei32x tile_w;
        sizei32y tile_h;
        uint32_t texture_id;
        int32_t  count;
        read_only_pointer_t pos_coords;
        read_only_pointer_t tex_coords;
        read_only_pointer_t colors;
    };

    struct tile_params_variable {
        uint32_t texture_id;
        int32_t  count;
        read_only_pointer_t pos_coords;
        read_only_pointer_t tex_coords;
        read_only_pointer_t tex_sizes;
        read_only_pointer_t colors;
    };

    struct transform_t {
        float scale_x;
        float scale_y;
        float trans_x;
        float trans_y;
    };

    template <typename T>
    class undo_t {
    public:
        undo_t(undo_t const&) = delete;
        undo_t& operator=(undo_t const&) = delete;

        undo_t(undo_t&& other) noexcept
          : action_ {std::move(other.action_)}
          , r_      {other.r_}
          , active_ {other.active_}
        {
            other.active_ = false;
        }

        undo_t& operator=(undo_t&& rhs) noexcept {
            std::swap(action_, rhs.action_);
            std::swap(active_, rhs.active_);
            return *this;
        }

        template <typename U>
        undo_t(renderer2d& r, U&& action) noexcept
          : action_ {std::forward<U>(action)}
          , r_ {r}
        {
        }

        ~undo_t() {
            if (active_) {
                action_(r_);
            }
        }

        void dismiss() noexcept { active_ = false; }
    private:
        T           action_;
        renderer2d& r_;
        bool        active_ = true;
    };

    struct undo_transform_action {
        transform_t trans;

        void operator()(renderer2d& r) {
            r.set_transform(trans);
        }
    };

    struct undo_clip_rect_action {
        recti32 rect;

        void operator()(renderer2d& r) {
            r.set_clip_rect(rect);
        }
    };

    using undo_transform = undo_t<undo_transform_action>;
    using undo_clip_rect = undo_t<undo_clip_rect_action>;

    virtual ~renderer2d();

    virtual recti32 get_client_rect() const = 0;

    virtual void set_clip_rect(recti32 r) = 0;
    virtual undo_clip_rect clip_rect(recti32 r) = 0;
    virtual void clip_rect() = 0;

    virtual void set_transform(transform_t t) = 0;
    virtual undo_transform transform(transform_t t) = 0;
    virtual void transform() = 0;

    virtual void render_clear()   = 0;
    virtual void render_present() = 0;

    virtual void fill_rect(recti32 r, uint32_t color) = 0;

    virtual void fill_rects(
        recti32  const* r_first, recti32  const* r_last
      , uint32_t const* c_first, uint32_t const* c_last) = 0;

    virtual void fill_rects(
        recti32  const* r_first, recti32  const* r_last
      , uint32_t color) = 0;

    virtual void draw_rect(recti32 r, int32_t border_size, uint32_t color) = 0;

    virtual void draw_rects(
        recti32  const* r_first, recti32  const* r_last
      , uint32_t const* c_first, uint32_t const* c_last
      , int32_t  border_size
    ) = 0;

    virtual void draw_rects(
        recti32  const* r_first, recti32  const* r_last
      , uint32_t color
      , int32_t  border_size
    ) = 0;

    virtual void draw_background() = 0;

    virtual void draw_tiles(tile_params_uniform  const& params) = 0;
    virtual void draw_tiles(tile_params_variable const& params) = 0;
};

std::unique_ptr<renderer2d> make_renderer(system& sys);

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
class render_task {
public:
    using clock_t     = std::chrono::high_resolution_clock;
    using timepoint_t = clock_t::time_point;
    using duration_t  = clock_t::duration;

    virtual ~render_task();
    virtual void render(duration_t delta, renderer2d& r, view const& v) = 0;
};

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
class tool_tip_renderer : public render_task {
public:
    virtual ~tool_tip_renderer();

    virtual bool is_visible() const noexcept = 0;
    virtual bool visible(bool state) noexcept = 0;

    virtual void set_text(std::string text) = 0;

    virtual void set_position(point2i32 p) noexcept = 0;
};

std::unique_ptr<tool_tip_renderer> make_tool_tip_renderer(text_renderer& tr);

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
class message_log_renderer : public render_task {
public:
    virtual ~message_log_renderer();

    virtual void resize(vec2i32 delta) = 0;

    virtual void show() = 0;
    virtual void fade(int32_t percent) = 0;

    virtual void scroll_pixels_v(int32_t pixels) = 0;
    virtual void scroll_lines_v(int32_t line) = 0;
    virtual void scroll_messages_v(int32_t messages) = 0;
    virtual void scroll_reset_v() = 0;
};

std::unique_ptr<message_log_renderer>
make_message_log_renderer(text_renderer& tr, message_log const& ml);

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
class item_list_renderer : public render_task {
public:
    virtual ~item_list_renderer();

    virtual bool set_focus(bool state) noexcept = 0;
};

std::unique_ptr<item_list_renderer>
make_item_list_renderer(text_renderer& tr, inventory_list const& il);

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
class map_renderer : public render_task {
public:
    template <typename T>
    struct update_t {
        point2i32 prev_pos;
        point2i32 next_pos;
        T         id;
    };

    virtual ~map_renderer();

    virtual bool debug_toggle_show_regions() noexcept = 0;

    virtual void highlight(point2i32 const* first, point2i32 const* last) = 0;
    virtual void highlight_clear() = 0;

    virtual void set_level(level const& lvl) noexcept = 0;
    virtual void set_tile_maps(std::initializer_list<std::pair<tile_map_type, tile_map const&>> tmaps) noexcept = 0;

    virtual void update_map_data() = 0;
    virtual void update_map_data(const_sub_region_range<tile_id> sub_region) = 0;

    virtual void update_data(update_t<entity_id> const* first
                           , update_t<entity_id> const* last) = 0;

    virtual void update_data(update_t<item_id> const* first
                           , update_t<item_id> const* last) = 0;

};

std::unique_ptr<map_renderer> make_map_renderer();

//=====--------------------------------------------------------------------=====
// Responsible for rendering all the various game and ui objects.
//=====--------------------------------------------------------------------=====
class game_renderer {
public:
    using duration_t = render_task::duration_t;

    virtual ~game_renderer();

    virtual void render(duration_t delta, view const& v) const noexcept = 0;

    template <typename T>
    T& add_task(
        string_view const  id
      , std::unique_ptr<T> task
      , int const          zorder
    ) {
        static_assert(std::is_base_of<render_task, T>::value, "");
        auto const result = task.get();
        add_task_generic(id, std::move(task), zorder);
        return *result;
    }

    virtual void add_task_generic(string_view id
                                , std::unique_ptr<render_task> task
                                , int zorder) = 0;
};

std::unique_ptr<game_renderer>
make_game_renderer(system& os, text_renderer& trender);

} //namespace boken
