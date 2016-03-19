#include "render.hpp"
#include "level.hpp"
#include "math.hpp"
#include "message_log.hpp"
#include "system.hpp"
#include "text.hpp"
#include "tile.hpp"
#include "utility.hpp"
#include "inventory.hpp"

#include <bkassert/assert.hpp>

#include <vector>
#include <cstdint>

namespace boken {

game_renderer::~game_renderer() = default;

class game_renderer_impl final : public game_renderer {
    static auto position_to_pixel_(tile_map const& tmap) noexcept {
        auto const tw = value_cast(tmap.tile_width());
        auto const th = value_cast(tmap.tile_height());

        return [tw, th](auto const p) noexcept {
            return point2<uint16_t> {
                static_cast<uint16_t>(value_cast(p.x) * tw)
              , static_cast<uint16_t>(value_cast(p.y) * th)};
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
                return data_t {
                    tranform_point(p)
                  , tmap.index_to_rect(index).top_left().template cast_to<uint16_t>()
                  , 0xFFu};
            });
    }

    template <typename Data, typename Updates>
    void update_data_(Data& data, Updates const& updates, tile_map const& tmap) {
        auto const tranform_point = position_to_pixel_(tmap);
        auto const get_texture_coord = [&](auto const& id) noexcept {
            return tmap.index_to_rect(id_to_index(tmap, id))
              .top_left().template cast_to<uint16_t>();
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
            it->position = tranform_point(update.next_pos);
            it->tex_coord = get_texture_coord(update.id);
        }
    }
public:
    game_renderer_impl(system& os, text_renderer& trender)
      : os_      {os}
      , trender_ {trender}
    {
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
    void update_tool_tip_position(point2i p) noexcept final override;

    void set_message_window(message_log const* const window) noexcept final override {
        message_log_ = window;
    }

    void set_inventory_window(inventory_list const* const window) noexcept final override {
        inventory_list_ = window;
    }

    void render(duration_t delta, view const& v) const noexcept final override;
private:
    void render_text_(text_layout const& text, vec2i off) const noexcept;
    void render_message_log_() const noexcept;
    void render_inventory_list_() const noexcept;

    system&        os_;
    text_renderer& trender_;

    level const* level_ {};

    struct data_t {
        point2<uint16_t> position;
        point2<uint16_t> tex_coord;
        uint32_t         color;
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

void game_renderer_impl::update_map_data(const_sub_region_range<tile_id> const sub_region) {
    auto const& tmap = *tile_map_items_;

    auto dst_it = sub_region_iterator<data_t>(sub_region.first, tile_data.data());

    for (auto it = sub_region.first; it != sub_region.second; ++it, ++dst_it) {
        auto& dst = *dst_it;

        auto const tex_rect = tmap.index_to_rect(id_to_index(tmap, *it));

        dst.tex_coord.x = offset_type_x<uint16_t> {tex_rect.x0};
        dst.tex_coord.y = offset_type_y<uint16_t> {tex_rect.y0};
    }
}

void game_renderer_impl::update_map_data() {
    auto const& tmap = *tile_map_base_;
    auto const& lvl  = *level_;

    auto const bounds = lvl.bounds();
    auto const bounds_size = static_cast<size_t>(std::max(0, bounds.area()));

    if (tile_data.size() < bounds_size) {
        tile_data.resize(bounds_size);
    }

    auto const region_color = [](uint32_t i) noexcept {
        ++i;
        return 0xFFu << 24 | (i * 11u) << 16 | (i * 23u) << 8 | (i * 37u);
    };

    auto const transform_point = position_to_pixel_(tmap);

    auto const ids_range        = lvl.tile_ids(bounds);
    auto const region_ids_range = lvl.region_ids(bounds);

    auto dst = sub_region_iterator<data_t> {ids_range.first, tile_data.data()};
    auto it0 = ids_range.first;
    auto it1 = region_ids_range.first;

    for ( ; it0 != ids_range.second; ++it0, ++it1, ++dst) {
        auto const tid = *it0;
        auto const rid = *it1;
        auto const index = id_to_index(tmap, tid);

        dst->position = transform_point(point2<ptrdiff_t> {dst.x(), dst.y()});
        dst->tex_coord = tmap.index_to_rect(index).top_left().cast_to<uint16_t>();
        dst->color = (tid == tile_id::empty)
          ? region_color(rid)
          : 0xFF0000FFu;
    }
}

void game_renderer_impl::update_tool_tip_text(std::string text) {
    tool_tip_.layout(trender_, std::move(text));
}

bool game_renderer_impl::update_tool_tip_visible(bool const show) noexcept {
    return tool_tip_.visible(show);
}

void game_renderer_impl::update_tool_tip_position(point2i const p) noexcept {
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
    render_text_(tool_tip_, vec2i {0, 0});

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

void game_renderer_impl::render_text_(text_layout const& text, vec2i const off) const noexcept {
    if (!text.is_visible()) {
        return;
    }

    text.update(trender_);

    auto const& glyph_data = text.data();

    auto const r  = text.extent() + off;
    auto const p  = r.top_left();
    auto const tx = value_cast<float>(p.x);
    auto const ty = value_cast<float>(p.y);

    os_.render_set_transform(1.0f, 1.0f, tx, ty);

    os_.render_fill_rect(r, 0xDF666666u);

    os_.render_set_data(render_data_type::position
      , read_only_pointer_t {glyph_data, BK_OFFSETOF(text_layout::data_t, position)});
    os_.render_set_data(render_data_type::texture
      , read_only_pointer_t {glyph_data, BK_OFFSETOF(text_layout::data_t, texture)});
    os_.render_set_data(render_data_type::color
      , read_only_pointer_t {glyph_data, BK_OFFSETOF(text_layout::data_t, color)});
    os_.render_data_n(glyph_data.size());
}

void game_renderer_impl::render_message_log_() const noexcept {
    if (!message_log_) {
        return;
    }

    auto const& log_window = *message_log_;
    auto const v = vec2i {0, 0};

    std::for_each(log_window.visible_begin(), log_window.visible_end()
      , [&](text_layout const& line) noexcept { render_text_(line, v); });
}

void game_renderer_impl::render_inventory_list_() const noexcept {
    if (!inventory_list_) {
        return;
    }

    auto const& inv_window = *inventory_list_;
    auto const v = inv_window.position() - point2i {0, 0};

    for (size_t i = 0; i < inv_window.cols(); ++i) {
        auto const info = inv_window.col(static_cast<int>(i));
        render_text_(info.text, v);
    }

    for (size_t i = 0; i < inv_window.rows(); ++i) {
        auto const range = inv_window.row(static_cast<int>(i));

        std::for_each(range.first, range.second, [&](text_layout const& txt) noexcept {
            render_text_(txt, v);
        });
    }
}

} //namespace boken
