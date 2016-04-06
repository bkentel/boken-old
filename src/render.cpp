#include "render.hpp"
#include "level.hpp"
#include "math.hpp"
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

game_renderer::~game_renderer() = default;

class game_renderer_impl final : public game_renderer {
    static auto position_to_pixel_(tile_map const& tmap) noexcept {
        auto const tw = value_cast(tmap.tile_width());
        auto const th = value_cast(tmap.tile_height());

        return [tw, th](auto const p) noexcept -> point2i16 {
            return {underlying_cast_unsafe<int16_t>(p.x * tw)
                  , underlying_cast_unsafe<int16_t>(p.y * th)};
        };
    }

    template <typename Data, typename Positions, typename Ids>
    void update_data_(Data& data, Positions const& ps, Ids const& ids, tile_map const& tmap) {
        auto const tranform_point = position_to_pixel_(tmap);

        data.clear();
        data.reserve(static_cast<size_t>(
            std::distance(ps.first, ps.second)));

        size_t i = 0;
        std::transform(ps.first, ps.second, back_inserter(data)
          , [&](auto const p) noexcept {
                auto const index = id_to_index(tmap, *(ids.first + i++));
                auto const tex_p = tmap.index_to_rect(index).top_left();
                return data_t {
                    tranform_point(p)
                  , underlying_cast_unsafe<int16_t>(tex_p)
                  , 0xFFu};
            });
    }

    template <typename Data, typename Updates>
    void update_data_(Data& data, Updates const& updates, tile_map const& tmap) {
        auto const tranform_point = position_to_pixel_(tmap);
        auto const get_texture_coord = [&](auto const& id) noexcept {
            return underlying_cast_unsafe<int16_t>(
                tmap.index_to_rect(id_to_index(tmap, id)).top_left());
        };

        auto first = begin(data);
        auto last  = end(data);

        for (auto const& update : updates) {
            auto const p = tranform_point(update.prev_pos);
            auto const it = std::find_if(first, last
              , [p](data_t const& d) noexcept { return d.position == p; });

            // new data
            if (it == last) {
                data.push_back({
                    p, get_texture_coord(update.id), 0xFF00FF00u});
                continue;
            }

            // data to remove
            if (update.id == nullptr) {
                data.erase(it);
                first = begin(data);
                last  = end(data);
                continue;
            }

            // data to update
            it->position  = tranform_point(update.next_pos);
            it->tex_coord = get_texture_coord(update.id);
        }
    }
public:
    game_renderer_impl(system& os, text_renderer& trender)
      : os_      {os}
      , trender_ {trender}
    {
    }

    bool debug_toggle_show_regions() noexcept final override {
        bool const result = debug_show_regions_;
        debug_show_regions_ = !debug_show_regions_;
        return result;
    }

    void set_level(level const& lvl) noexcept final override {
        entity_data.clear();
        item_data.clear();
        tile_data.clear();
        level_ = &lvl;
    }

    void set_tile_maps(std::initializer_list<std::pair<tile_map_type, tile_map const&>> tmaps) noexcept final override;

    void update_map_data() final override;
    void update_map_data(const_sub_region_range<tile_id> sub_region) final override;

    void update_entity_data() final override {
        update_data_(entity_data
                   , level_->entity_positions()
                   , level_->entity_ids()
                   , *tile_map_entities_);
    }

    void update_item_data() final override {
        update_data_(item_data
                   , level_->item_positions()
                   , level_->item_ids()
                   , *tile_map_items_);
    }

    void update_entity_data(std::vector<update_t<entity_id>> const& updates) final override {
        update_data_(entity_data, updates, *tile_map_entities_);
    }

    void update_item_data(std::vector<update_t<item_id>> const& updates) final override {
        update_data_(item_data, updates, *tile_map_items_);
    }

    void update_tool_tip_text(std::string text) final override;
    bool update_tool_tip_visible(bool show) noexcept final override;
    void update_tool_tip_position(point2i32 p) noexcept final override;

    void set_message_window(message_log const* const window) noexcept final override {
        message_log_ = window;
    }

    void set_inventory_window(inventory_list const* const window) noexcept final override {
        inventory_list_ = window;
    }

    void render(duration_t delta, view const& v) const noexcept final override;
private:
    uint32_t choose_tile_color_(tile_id tid, region_id rid) noexcept;

    void render_text_(text_layout const& text, vec2i32 off) const noexcept;
    void render_tool_tip_() const noexcept;
    void render_message_log_() const noexcept;
    void render_inventory_list_() const noexcept;

    system&        os_;
    text_renderer& trender_;

    level const* level_ {};

    struct data_t {
        point2i16 position;
        point2i16 tex_coord;
        uint32_t  color;
    };

    tile_map const* tile_map_base_ {};
    tile_map const* tile_map_entities_ {};
    tile_map const* tile_map_items_ {};

    std::vector<data_t> tile_data;
    std::vector<data_t> entity_data;
    std::vector<data_t> item_data;

    text_layout tool_tip_;
    message_log const* message_log_ {};
    inventory_list const* inventory_list_ {};

    bool debug_show_regions_ = false;
};

std::unique_ptr<game_renderer> make_game_renderer(system& os, text_renderer& trender) {
    return std::make_unique<game_renderer_impl>(os, trender);
}

void game_renderer_impl::set_tile_maps(
    std::initializer_list<std::pair<tile_map_type, tile_map const&>> tmaps
) noexcept {
    for (auto const& p : tmaps) {
        switch (p.first) {
        case tile_map_type::base   : this->tile_map_base_     = &p.second; break;
        case tile_map_type::entity : this->tile_map_entities_ = &p.second; break;
        case tile_map_type::item   : this->tile_map_items_    = &p.second; break;
        default :
            break;
        }
    }
}

uint32_t game_renderer_impl::choose_tile_color_(tile_id const tid, region_id const rid) noexcept {
    if (debug_show_regions_) {
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
}

void game_renderer_impl::update_map_data(const_sub_region_range<tile_id> const sub_region) {
    auto const& tmap = *tile_map_items_;

    auto dst_it = sub_region_iterator<data_t>(sub_region.first, tile_data.data());

    auto const region_range = level_->region_ids({
        point2i32 {
            static_cast<int32_t>(dst_it.off_x())
          , static_cast<int32_t>(dst_it.off_y())}
      , sizei32x {static_cast<int32_t>(dst_it.width())}
      , sizei32y {static_cast<int32_t>(dst_it.height())}});


    auto rgn_it = region_range.first;

    for (auto it = sub_region.first; it != sub_region.second; ++it, ++dst_it, ++rgn_it) {
        auto& dst = *dst_it;

        auto const tid = *it;
        auto const rid = *rgn_it;

        auto const tex_rect = tmap.index_to_rect(id_to_index(tmap, tid));
        dst.tex_coord = underlying_cast_unsafe<int16_t>(tex_rect.top_left());
        dst.color     = choose_tile_color_(tid, rid);
    }
}

void game_renderer_impl::update_map_data() {
    auto const& tmap = *tile_map_base_;
    auto const& lvl  = *level_;

    auto const bounds = lvl.bounds();
    auto const bounds_size = static_cast<size_t>(
        std::max(0, value_cast(bounds.area())));

    if (tile_data.size() < bounds_size) {
        tile_data.resize(bounds_size);
    }

    auto const transform_point = position_to_pixel_(tmap);

    auto const ids_range        = lvl.tile_ids(bounds);
    auto const region_ids_range = lvl.region_ids(bounds);

    auto dst = sub_region_iterator<data_t> {ids_range.first, tile_data.data()};
    auto it0 = ids_range.first;
    auto it1 = region_ids_range.first;

    for ( ; it0 != ids_range.second; ++it0, ++it1, ++dst) {
        auto const tid   = *it0;
        auto const rid   = *it1;
        auto const index = id_to_index(tmap, tid);

        auto const p = transform_point(point2<ptrdiff_t> {dst.x(), dst.y()});

        dst->position  = underlying_cast_unsafe<int16_t>(p);
        dst->tex_coord = underlying_cast_unsafe<int16_t>(tmap.index_to_rect(index).top_left());
        dst->color     = choose_tile_color_(tid, rid);
    }
}

void game_renderer_impl::update_tool_tip_text(std::string text) {
    tool_tip_.layout(trender_, std::move(text));
}

bool game_renderer_impl::update_tool_tip_visible(bool const show) noexcept {
    return tool_tip_.visible(show);
}

void game_renderer_impl::update_tool_tip_position(point2i32 const p) noexcept {
    tool_tip_.move_to(value_cast(p.x), value_cast(p.y));
}

void game_renderer_impl::render(duration_t const delta, view const& v) const noexcept {
    auto const& tmap_base     = *tile_map_base_;
    auto const& tmap_entities = *tile_map_entities_;
    auto const& tmap_items    = *tile_map_items_;

    os_.render_clear();

    os_.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);
    os_.render_background();

    os_.render_set_transform(v.scale_x, v.scale_y, v.x_off, v.y_off);

    //
    // Map tiles
    //
    os_.render_set_tile_size(tmap_base.tile_width(), tmap_base.tile_height());
    os_.render_set_texture(tmap_base.texture_id());

    os_.render_set_data(render_data_type::position
      , read_only_pointer_t {tile_data, BK_OFFSETOF(data_t, position)});
    os_.render_set_data(render_data_type::texture
      , read_only_pointer_t {tile_data, BK_OFFSETOF(data_t, tex_coord)});
    os_.render_set_data(render_data_type::color
      , read_only_pointer_t {tile_data, BK_OFFSETOF(data_t, color)});
    os_.render_data_n(tile_data.size());

    //
    // Items
    //
    os_.render_set_tile_size(tmap_items.tile_width(), tmap_items.tile_height());
    os_.render_set_texture(tmap_items.texture_id());

    os_.render_set_data(render_data_type::position
      , read_only_pointer_t {item_data, BK_OFFSETOF(data_t, position)});
    os_.render_set_data(render_data_type::texture
      , read_only_pointer_t {item_data, BK_OFFSETOF(data_t, tex_coord)});
    os_.render_set_data(render_data_type::color
      , read_only_pointer_t {item_data, BK_OFFSETOF(data_t, color)});
    os_.render_data_n(item_data.size());

    //
    // Entities
    //
    os_.render_set_tile_size(tmap_entities.tile_width(), tmap_entities.tile_height());
    os_.render_set_texture(tmap_entities.texture_id());

    os_.render_set_data(render_data_type::position
      , read_only_pointer_t {entity_data, BK_OFFSETOF(data_t, position)});
    os_.render_set_data(render_data_type::texture
      , read_only_pointer_t {entity_data, BK_OFFSETOF(data_t, tex_coord)});
    os_.render_set_data(render_data_type::color
      , read_only_pointer_t {entity_data, BK_OFFSETOF(data_t, color)});
    os_.render_data_n(entity_data.size());

    //
    // text
    //
    os_.render_set_tile_size(tmap_base.tile_width(), tmap_base.tile_height());
    os_.render_set_texture(3);

    //
    // tool tip
    //
    render_tool_tip_();

    //
    // message log window
    //
    render_message_log_();

    //
    // inventory window
    //
    render_inventory_list_();

    os_.render_present();
}

void game_renderer_impl::render_text_(text_layout const& text, vec2i32 const off) const noexcept {
    if (!text.is_visible()) {
        return;
    }

    text.update(trender_);

    auto const& glyph_data = text.data();

    auto const r  = text.extent() + off;
    auto const p  = r.top_left();
    auto const tx = value_cast_unsafe<float>(p.x);
    auto const ty = value_cast_unsafe<float>(p.y);

    os_.render_set_transform(1.0f, 1.0f, tx, ty);

    os_.render_set_data(render_data_type::position
      , read_only_pointer_t {glyph_data, BK_OFFSETOF(text_layout::data_t, position)});
    os_.render_set_data(render_data_type::texture
      , read_only_pointer_t {glyph_data, BK_OFFSETOF(text_layout::data_t, texture)});
    os_.render_set_data(render_data_type::color
      , read_only_pointer_t {glyph_data, BK_OFFSETOF(text_layout::data_t, color)});
    os_.render_data_n(glyph_data.size());
}

void game_renderer_impl::render_tool_tip_() const noexcept {
    if (!tool_tip_.is_visible()) {
        return;
    }

    auto const r = tool_tip_.extent();

    os_.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);
    os_.render_fill_rect(r, 0xDF666666u);

    render_text_(tool_tip_, vec2i32 {});
}

void game_renderer_impl::render_message_log_() const noexcept {
    if (!message_log_) {
        return;
    }

    auto const& log_window = *message_log_;

    recti32 const r {
        point2i32 {}
      , sizei32x {500}
      , sizei32y {static_cast<int32_t>(log_window.max_visible()) * trender_.line_gap()}
    };

    os_.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);
    os_.render_fill_rect(r, 0xDF666666u);

    auto const v = vec2i32 {};
    std::for_each(log_window.visible_begin(), log_window.visible_end()
      , [&](text_layout const& line) noexcept { render_text_(line, v); });
}

void game_renderer_impl::render_inventory_list_() const noexcept {
    if (!inventory_list_ || !inventory_list_->is_visible()) {
        return;
    }

    auto const& inv_window = *inventory_list_;
    auto const  m = inv_window.metrics();

    //TODO: move to renderer
    auto const draw_hollow_rect = [&](recti32 const r, int32_t const w, int32_t const h) {
        auto const rw = value_cast(r.width());
        auto const rh = value_cast(r.height());

        auto const x0 = value_cast(r.x0);
        auto const y0 = value_cast(r.y0);
        auto const x1 = value_cast(r.x1);
        auto const y1 = value_cast(r.y1);

        auto const color = 0xDF666666u;

        auto const r_left   = recti32 {point2i32 {x0,     y0}, sizei32x {w}, sizei32y {rh}};
        auto const r_right  = recti32 {point2i32 {x1 - w, y0}, sizei32x {w}, sizei32y {rh}};
        auto const r_top    = recti32 {point2i32 {x0 + w, y0}, sizei32x {rw - 2*w}, sizei32y {h}};
        auto const r_bottom = recti32 {point2i32 {x0 + w, y1 - h}, sizei32x {rw - 2*w}, sizei32y {h}};

        os_.render_fill_rect(r_left, color);
        os_.render_fill_rect(r_right, color);
        os_.render_fill_rect(r_top, color);
        os_.render_fill_rect(r_bottom, color);
    };

    // draw the frame
    {
        auto const frame_size = (m.frame.width() - m.client_frame.width()) / 2;
        os_.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);
        draw_hollow_rect(m.frame, value_cast(frame_size), value_cast(frame_size));
    }

    // draw the title
    {
        os_.render_fill_rect(m.title, 0xEF886666u);
        render_text_(inv_window.title(), m.title.top_left() - point2i32 {});
    }

    // draw the client area
    if (inv_window.cols() <= 0) {
        return;
    }

    os_.render_set_clip_rect(m.client_frame);
    auto on_exit = BK_SCOPE_EXIT {
        os_.render_clear_clip_rect();
    };

    auto const v = m.client_frame.top_left() - point2i32 {};
    os_.render_fill_rect({point2i32 {} + v, m.client_frame.width(), m.header_h}, 0xDF66AA66u);

    for (size_t i = 0; i < inv_window.cols(); ++i) {
        auto const info = inv_window.col(static_cast<int>(i));
        render_text_(info.text, v);
    }

    for (size_t i = 0; i < inv_window.rows(); ++i) {
        auto const range = inv_window.row(static_cast<int>(i));

        auto const p = range.first->position() + v;
        auto const w = m.client_frame.width();
        auto const h = range.first->extent().height();

        os_.render_fill_rect({p, w, h}, 0xDF666666u);

        std::for_each(range.first, range.second, [&](text_layout const& txt) noexcept {
            render_text_(txt, v);
        });
    }

}

} //namespace boken
