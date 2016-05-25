#include "render.hpp"
#include "level.hpp"
#include "math.hpp"
#include "rect.hpp"
#include "message_log.hpp"
#include "system.hpp"
#include "text.hpp"
#include "tile.hpp"
#include "utility.hpp"
#include "inventory.hpp"
#include "scope_guard.hpp"

#include <bkassert/assert.hpp>

#include <vector>
#include <cstdint>

namespace boken {

namespace {

void render_text(
    renderer2d& r
  , text_renderer& tr
  , text_layout const& text
  , vec2i32 const off
) noexcept {
    if (!text.is_visible()) {
        return;
    }

    text.update(tr);

    auto const& glyph_data = text.data();

    auto const p  = (text.extent() + off).top_left();
    auto const tx = value_cast_unsafe<float>(p.x);
    auto const ty = value_cast_unsafe<float>(p.y);

    using ptr_t = read_only_pointer_t;
    auto const params = renderer2d::tile_params_variable {
        3
      , static_cast<int32_t>(glyph_data.size())
      , ptr_t {glyph_data, BK_OFFSETOF(text_layout::data_t, position)}
      , ptr_t {glyph_data, BK_OFFSETOF(text_layout::data_t, texture)}
      , ptr_t {glyph_data, BK_OFFSETOF(text_layout::data_t, size)}
      , ptr_t {glyph_data, BK_OFFSETOF(text_layout::data_t, color)}
    };

    auto const trans = r.transform({1.0f, 1.0f, tx, ty});
    r.draw_tiles(params);
}

} //namespace

render_task::~render_task() = default;

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
tool_tip_renderer::~tool_tip_renderer() = default;

class tool_tip_renderer_impl final : public tool_tip_renderer {
public:
    tool_tip_renderer_impl(text_renderer& tr)
      : trender_ {tr}
    {
    }

    //---render_task interface
    void render(duration_t delta, renderer2d& r, view const& v) final override;

    //---tool_tip_renderer interface
    bool is_visible() const noexcept final override {
        return text_.is_visible();
    }

    bool visible(bool const state) noexcept final override {
        return text_.visible(state);
    }

    void set_text(std::string text) final override {
        text_.layout(trender_, std::move(text));
    }

    void set_position(point2i32 const p) noexcept final override {
        text_.move_to(value_cast(p.x), value_cast(p.y));
    }
private:
    text_renderer& trender_;
    text_layout    text_;
};

std::unique_ptr<tool_tip_renderer> make_tool_tip_renderer(text_renderer& tr) {
    return std::make_unique<tool_tip_renderer_impl>(tr);
}

void tool_tip_renderer_impl::render(duration_t, renderer2d& r, view const&) {
    if (!is_visible()) {
        return;
    }

    auto const border_w = 2;
    auto const window_r = r.get_client_rect();
    auto const text_r   = text_.extent();
    auto const border_r = grow_rect(text_r, border_w);

    auto const dx = (border_r.x1 > window_r.x1)
      ? value_cast(window_r.x1 - border_r.x1)
      : 0;

    auto const dy = (border_r.y1 > window_r.y1)
      ? value_cast(window_r.y1 - border_r.y1)
      : 0;

    auto const v = vec2i32 {dx, dy};

    auto const trans = r.transform({1.0f, 1.0f, 0.0f, 0.0f});

    r.fill_rect(text_r + v, 0xDF666666u);
    r.draw_rect(border_r + v, border_w, 0xDF66DDDDu);

    render_text(r, trender_, text_, v);
}

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
message_log_renderer::~message_log_renderer() = default;

class message_log_renderer_impl final : public message_log_renderer {
public:
    message_log_renderer_impl(text_renderer& tr, message_log const& log) noexcept
      : log_ {&log}
      , trender_ {tr}
    {
    }

    //---render_task interface
    void render(duration_t delta, renderer2d& r, view const& v) final override;

    //---message_log_renderer interface
    void resize(vec2i32 const delta) final override {
    }

    void show() final override {
        fading_ = false;
        fade_time_ = duration_t {};
    }

    void fade(int32_t const percent) final override {
    }

    void scroll_pixels_v(int32_t const pixels) final override {
    }

    void scroll_lines_v(int32_t const line) final override {
    }

    void scroll_messages_v(int32_t const messages) final override {
    }

    void scroll_reset_v() final override {
    }
private:
    message_log const* log_;
    text_renderer& trender_;
    bool fading_ = false;
    duration_t fade_time_ {};
};

std::unique_ptr<message_log_renderer> make_message_log_renderer(
    text_renderer& tr
  , message_log const& ml
) {
    return std::make_unique<message_log_renderer_impl>(tr, ml);
}

void message_log_renderer_impl::render(duration_t const delta, renderer2d& r, view const&) {
    if (!log_) {
        return;
    }

    constexpr std::chrono::milliseconds fade_time      {3000};
    constexpr std::chrono::milliseconds fade_lead_time {1000};
    constexpr auto fade_total_time = fade_time + fade_lead_time;

    if (fading_ == false) {
        fading_ = true;
        fade_time_ = duration_t {};
    } else if (fade_time_ < fade_total_time) {
        fade_time_ += delta;
    }

    auto const& log_window = *log_;

    auto const bounds   = log_window.bounds();
    auto const client_r = log_window.client_bounds();

    auto const v = [&]() noexcept {
        auto const ch = client_r.height();
        auto const rh = bounds.height();

        return (ch <= rh)
          ? vec2i32 {}
          : vec2i32 {0, rh - ch};
    }();

    auto const trans = r.transform({1.0f, 1.0f, 0.0f, 0.0f});

    auto const t0 = std::chrono::duration<float, std::milli> {fade_time_ - fade_lead_time};
    auto const t1 = t0 / fade_time;

    auto const scale = 1.0f - clamp(t1, 0.1f, 0.9f);
    auto const alpha = round_as<uint32_t>(255.0f * scale) & 0xFFu;
    auto const color = (alpha << 24) | 0x00666666u;

    r.fill_rect(bounds, color);

    std::for_each(log_window.visible_begin(), log_window.visible_end()
      , [&](text_layout const& line) noexcept {
            if (line.extent().y1 + v.y <= bounds.y0) {
                return;
            }

            render_text(r, trender_, line, v);
        });
}

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
item_list_renderer::~item_list_renderer() = default;

class item_list_renderer_impl final : public item_list_renderer {
public:
    item_list_renderer_impl(text_renderer& tr, inventory_list const& il)
      : trender_ {tr}
      , list_    {&il}
    {
    }

    //---render_task interface
    void render(duration_t delta, renderer2d& r, view const& v) final override;

    //---tool_tip_renderer interface
    bool set_focus(bool const state) noexcept final override {
        auto const result = has_focus_;
        has_focus_ = state;
        return result;
    }
private:
    text_renderer& trender_;
    inventory_list const* list_;
    bool has_focus_ = false;
};

std::unique_ptr<item_list_renderer>
make_item_list_renderer(text_renderer& tr, inventory_list const& il) {
    return std::make_unique<item_list_renderer_impl>(tr, il);
}

void item_list_renderer_impl::render(duration_t, renderer2d& r, view const&) {
    if (!list_ || !list_->is_visible()) {
        return;
    }

    auto const& inv_window = *list_;
    auto const  m          = inv_window.metrics();

    uint32_t const color_border       = 0xEF555555u;
    uint32_t const color_border_focus = 0xEFEFEFEFu;
    uint32_t const color_title        = 0xEF886666u;
    uint32_t const color_header       = 0xDF66AA66u;
    uint32_t const color_row_even     = 0xDF666666u;
    uint32_t const color_row_odd      = 0xDF888888u;
    uint32_t const color_row_sel      = 0xDFBB2222u;
    uint32_t const color_row_ind      = 0xDF22BBBBu;

    auto const trans = r.transform({1.0f, 1.0f, 0.0f, 0.0f});

    // draw the frame
    {
        auto const frame_size = (m.frame.width() - m.client_frame.width()) / 2;
        auto const color = has_focus_
          ? color_border_focus
          : color_border;

        r.draw_rect(m.frame, value_cast(frame_size), color);
    }

    // draw the title
    {
        r.fill_rect(m.title, color_title);
        render_text(r, trender_, inv_window.title(), m.title.top_left() - point2i32 {});
    }

    // draw the client area
    if (inv_window.cols() <= 0) {
        return; // TODO
    }

    // fill in any gap between the title and the client area
    auto const gap = m.client_frame.y0 - m.title.y1;
    if (gap > sizei32y {0}) {
        auto const gap_r = recti32 {
            m.client_frame.x0
          , m.title.y1
          , m.client_frame.width()
          , gap
        };

        r.fill_rect(gap_r, color_row_even);
    }

    auto const clip = r.clip_rect(m.client_frame);

    auto const v = (m.client_frame.top_left() - point2i32 {})
                 - inv_window.scroll_offset();

    //column separators
    for (size_t i = 0; i < inv_window.cols(); ++i) {
        auto const info = inv_window.col(static_cast<int>(i));
        auto const line = recti32 {
            info.text.position().x + info.width + v.x
          , m.client_frame.y0
          , sizei32x {2}
          , m.client_frame.height()
        };

        r.fill_rect(line, 0xEFFFFFFF);
    }

    // header background
    r.fill_rect({point2i32 {} + v, m.client_frame.width(), m.header_h}, color_header);

    int32_t last_y = value_cast(m.client_frame.y0);

    for (size_t i = 0; i < inv_window.cols(); ++i) {
        auto const info = inv_window.col(static_cast<int>(i));
        render_text(r, trender_, info.text, v);
        last_y = std::max(last_y, value_cast(info.text.extent().y1 + v.y));
    }

    auto const indicated = static_cast<size_t>(inv_window.indicated());

    for (size_t i = 0; i < inv_window.rows(); ++i) {
        auto const range = inv_window.row(static_cast<int>(i));

        auto const p = range.first->position() + v;
        auto const w = m.client_frame.width();
        auto const h = range.first->extent().height();

        auto const color =
            (inv_window.is_selected(static_cast<int>(i))) ? color_row_sel
          : (i % 2u)                                      ? color_row_even
                                                          : color_row_odd;

        // row background
        auto const row = recti32 {p, w, h};
        r.fill_rect(row, color);

        if (i == indicated) {
            r.draw_rect(row, 2, color_row_ind);
        }

        std::for_each(range.first, range.second, [&](text_layout const& txt) noexcept {
            render_text(r, trender_, txt, v);
        });

        last_y = std::max(last_y, value_cast(p.y + h));
        if (last_y >= value_cast(m.client_frame.y1)) {
            break;
        }
    }

    // fill unused background
    if (last_y < value_cast(m.client_frame.y1)) {
        auto const left_over = recti32 {
            m.client_frame.x0
          , offi32y {last_y}
          , m.client_frame.width()
          , m.client_frame.y1 - offi32y {last_y}
        };

        r.fill_rect(left_over, color_row_even);
    }
}

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
map_renderer::~map_renderer() = default;

class map_renderer_impl final : public map_renderer {
public:
    //---render_task interface
    void render(duration_t delta, renderer2d& r, view const& v) final override;

    //---map_renderer interface
    bool debug_toggle_show_regions() noexcept final override {
        bool const result = debug_show_regions_;
        debug_show_regions_ = !debug_show_regions_;
        return result;
    }

    void highlight(
        point2i32 const* const first
      , point2i32 const* const last
    ) final override {
        BK_ASSERT(!!first && !!last);

        auto const n = std::distance(first, last);
        BK_ASSERT(n >= 0);

        highlight_clear();
        highlighted_tiles_.reserve(static_cast<size_t>(n));

        std::copy(first, last, back_inserter(highlighted_tiles_));
    }

    void highlight_clear() final override {
        highlighted_tiles_.clear();
    }

    void set_level(level const& lvl) noexcept final override {
        if (level_ == &lvl) {
            return;
        }

        entity_data.clear();
        item_data.clear();
        tile_data.clear();
        highlight_clear();

        level_ = &lvl;
    }

    void set_tile_maps(
        std::initializer_list<std::pair<tile_map_type, tile_map const&>> tmaps
    ) noexcept final override {
        for (auto const& p : tmaps) {
            switch (p.first) {
            case tile_map_type::base   : tile_map_base_     = &p.second; break;
            case tile_map_type::entity : tile_map_entities_ = &p.second; break;
            case tile_map_type::item   : tile_map_items_    = &p.second; break;
            default :
                break;
            }
        }
    }

    void update_map_data() final override;
    void update_map_data(const_sub_region_range<tile_id> sub_region) final override;

    void update_data(
        update_t<entity_id> const* first
      , update_t<entity_id> const* last
    ) final override {
        update_data_(entity_data, first, last, *tile_map_entities_);
    }

    void update_data(
        update_t<item_id> const* first
      , update_t<item_id> const* last
    ) final override {
        update_data_(item_data, first, last, *tile_map_items_);
    }
private:
    struct data_t {
        point2i16 position;
        point2i16 tex_coord;
        uint32_t  color;
    };

    static auto tile_pos_to_rect_(tile_map const& tmap) noexcept {
        auto const w  = tmap.tile_width();
        auto const h  = tmap.tile_height();
        auto const w0 = value_cast(w);
        auto const h0 = value_cast(h);

        return [=](point2i32 const p) noexcept -> recti32 {
            return {p.x * w0, p.y * h0, w, h};
        };
    }

    template <typename Data, typename T>
    static renderer2d::tile_params_uniform
    make_uniform(tile_map const& tmap, T const& data) noexcept {
        using ptr_t = read_only_pointer_t;
        return {
            tmap.tile_width(), tmap.tile_height(), tmap.texture_id()
          , static_cast<int32_t>(data.size())
          , ptr_t {data, BK_OFFSETOF(Data, position)}
          , ptr_t {data, BK_OFFSETOF(Data, tex_coord)}
          , ptr_t {data, BK_OFFSETOF(Data, color)}
        };
    }

    static auto position_to_pixel_(tile_map const& tmap) noexcept {
        auto const tw = value_cast(tmap.tile_width());
        auto const th = value_cast(tmap.tile_height());

        return [tw, th](auto const p) noexcept -> point2i16 {
            return {underlying_cast_unsafe<int16_t>(p.x * tw)
                  , underlying_cast_unsafe<int16_t>(p.y * th)};
        };
    }

    static auto get_tex_coord(tile_map const& tmap) noexcept {
        return [&](auto const id) {
            return underlying_cast_unsafe<int16_t>(
                tmap.index_to_rect(id_to_index(tmap, id)).top_left());
        };
    }

    auto choose_tile_color_() noexcept {
        return [show_debug = debug_show_regions_]
               (tile_id const tid, region_id const rid) noexcept -> uint32_t
        {
            if (show_debug) {
                auto const n = value_cast(rid) + 1u;

                return (0xFFu     << 24)  //A
                     | ((n * 11u) << 16)  //B
                     | ((n * 23u) <<  8)  //G
                     | ((n * 37u) <<  0); //R
            }

            if (tid == tile_id::empty) {
                return 0xFF222222u;
            }

            return 0xFFAAAAAAu;
        };
    }

    template <typename SetData>
    void update_map_data_(
        const_sub_region_range<tile_id>   tids
      , const_sub_region_range<region_id> rids
      , sub_region_iterator<data_t>       out
      , SetData set
    ) {
        auto it_tid = tids.first;
        auto it_rid = rids.first;

        for ( ; it_tid != tids.second; ++it_tid, ++it_rid, ++out) {
            set(*out, make_point2(out.x(), out.y()), *it_tid, *it_rid);
        }
    }

    template <typename Data, typename Type>
    void update_data_(
        Data&                       data
      , update_t<Type> const* const first
      , update_t<Type> const* const last
      , tile_map const&             tmap
    ) {
        auto const tranform  = position_to_pixel_(tmap);
        auto const tex_coord = get_tex_coord(tmap);
        auto const get_color = [&](update_t<Type> const&) noexcept {
            return 0xFF00FF00u;
        };

        std::for_each(first, last, [&](update_t<Type> const& update) {
            auto const p = tranform(update.prev_pos);

            auto const first_d = begin(data);
            auto const last_d  = end(data);

            auto const it = std::find_if(first_d, last_d
              , [&](data_t const& d) noexcept { return d.position == p; });

            // data to remove
            if (update.id == nullptr) {
                BK_ASSERT(it != last_d);

                // equivalent to  data.erase(it)
                std::swap(*it, data.back());
                data.pop_back();

                return;
            }

            // new data
            if (it == last_d) {
                data.push_back({p, tex_coord(update.id), get_color(update)});
                return;
            }

            // data to update
            it->position  = tranform(update.next_pos);
            it->tex_coord = tex_coord(update.id);
            it->color     = get_color(update);
        });
    }
private:
    level const* level_ {};

    std::vector<data_t> tile_data;
    std::vector<data_t> entity_data;
    std::vector<data_t> item_data;

    tile_map const* tile_map_base_     {};
    tile_map const* tile_map_entities_ {};
    tile_map const* tile_map_items_    {};

    std::vector<point2i32> highlighted_tiles_;

    bool debug_show_regions_ = false;
};

std::unique_ptr<map_renderer> make_map_renderer() {
    return std::make_unique<map_renderer_impl>();
}

void map_renderer_impl::render(duration_t, renderer2d& r, view const& v) {
     auto const trans = r.transform({v.scale_x, v.scale_y, v.x_off, v.y_off});

    // Map tiles
    r.draw_tiles(make_uniform<data_t>(*tile_map_base_, tile_data));

    // Items
    r.draw_tiles(make_uniform<data_t>(*tile_map_items_, item_data));

    // Entities
    r.draw_tiles(make_uniform<data_t>(*tile_map_entities_, entity_data));

    // tile highlight
    auto const border_size = 2;
    auto const get_rect = tile_pos_to_rect_(*tile_map_base_);

    for (auto const& p : highlighted_tiles_) {
        r.draw_rect(grow_rect(get_rect(p), border_size), border_size, 0xD000FFFFu);
    }
}

void map_renderer_impl::update_map_data() {
    auto const& tmap   = *tile_map_base_;
    auto const& lvl    = *level_;
    auto const  bounds = lvl.bounds();

    // reserve enough space for the entire level
    {
        auto const bounds_size = value_cast_unsafe<size_t>(bounds.area());
        if (tile_data.size() < bounds_size) {
            tile_data.resize(bounds_size);
        }
    }

    auto const transform_point = position_to_pixel_(tmap);
    auto const choose_color    = choose_tile_color_();
    auto const tex_coord       = get_tex_coord(tmap);

    auto const tids = lvl.tile_ids(bounds);
    update_map_data_(
        tids
      , lvl.region_ids(bounds)
      , sub_region_iterator<data_t> {tids.first, tile_data.data()}
      , [&](data_t& out, auto const p, tile_id const tid, region_id const rid) {
            out.position  = transform_point(p);
            out.tex_coord = tex_coord(tid);
            out.color     = choose_color(tid, rid);
        });
}

void map_renderer_impl::update_map_data(
    const_sub_region_range<tile_id> const sub_region
) {
    auto dst = sub_region_iterator<data_t>(sub_region.first, tile_data.data());

    auto const x = static_cast<int32_t>(dst.off_x());
    auto const y = static_cast<int32_t>(dst.off_y());
    auto const w = static_cast<int32_t>(dst.width());
    auto const h = static_cast<int32_t>(dst.height());

    auto const rids =
        level_->region_ids({point2i32 {x, y}, sizei32x {w}, sizei32y {h}});

    auto const choose_color = choose_tile_color_();
    auto const tex_coord    = get_tex_coord(*tile_map_base_);

    update_map_data_(sub_region, rids, dst
      , [&](data_t& out, auto, tile_id const tid, region_id const rid) {
            out.tex_coord = tex_coord(tid);
            out.color     = choose_color(tid, rid);
        });
}

//=====--------------------------------------------------------------------=====
//=====--------------------------------------------------------------------=====
game_renderer::~game_renderer() = default;

class game_renderer_impl final : public game_renderer {
public:
    game_renderer_impl(system& os, text_renderer& trender)
      : os_      {os}
      , trender_ {trender}
    {
    }

    void render(duration_t delta, view const& v) const noexcept final override;

    void add_task_generic(
        string_view const id
      , std::unique_ptr<render_task> task
      , int const zorder
    ) final override {
        BK_ASSERT(!!task && !id.empty());
        tasks_.push_back({std::move(task), id, zorder});
    }
private:
    struct task_info {
        std::unique_ptr<render_task> task;
        string_view id;
        int zorder;
    };

    system&        os_;
    text_renderer& trender_;

    std::unique_ptr<renderer2d> renderer_ = make_renderer(os_);
    std::vector<task_info> tasks_;
};

std::unique_ptr<game_renderer> make_game_renderer(system& os, text_renderer& trender) {
    return std::make_unique<game_renderer_impl>(os, trender);
}

void game_renderer_impl::render(duration_t const delta, view const& v) const noexcept {
    auto& r = *renderer_;

    r.render_clear();
    r.transform();
    r.draw_background();

    for (auto const& t : tasks_) {
        t.task->render(delta, r, v);
    }

    r.render_present();
}

} //namespace boken
