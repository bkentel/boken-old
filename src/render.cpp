#include "render.hpp"
#include "level.hpp"
#include "math.hpp"
#include "message_log.hpp"
#include "system.hpp"
#include "text.hpp"
#include "tile.hpp"
#include "utility.hpp"

#include <bkassert/assert.hpp>

#include <vector>
#include <cstdint>

namespace boken {

game_renderer::~game_renderer() = default;

class game_renderer_impl final : public game_renderer {
public:
    game_renderer_impl(system& os, text_renderer& trender)
      : os_      {os}
      , trender_ {trender}
    {
    }

    void update_map_data(level const& lvl, tile_map const& tmap) final override;
    void update_map_data(const_sub_region_range<tile_id> sub_region, tile_map const& tmap) final override;
    void update_entity_data(level const& lvl, tile_map const& tmap) final override;

    void update_tool_tip_text(std::string text) final override;
    void update_tool_tip_visible(bool show) noexcept final override;
    void update_tool_tip_position(point2i p) noexcept final override;

    void set_message_window(message_log const* const window) noexcept final override {
        message_log_ = window;
    }

    void render(duration_t delta, view const& v
      , tile_map const& tmap_base
      , tile_map const& tmap_entities
      , tile_map const& tmap_items) const noexcept final override;
private:
    void render_text_(text_layout const& text) const noexcept;
    void render_message_log_() const noexcept;

    system&        os_;
    text_renderer& trender_;

    struct data_t {
        point2<uint16_t> position;
        point2<uint16_t> tex_coord;
        uint32_t         color;
    };

    std::vector<data_t> tile_data;
    std::vector<data_t> entity_data;

    text_layout tool_tip_;
    message_log const* message_log_ {};
};

std::unique_ptr<game_renderer> make_game_renderer(system& os, text_renderer& trender) {
    return std::make_unique<game_renderer_impl>(os, trender);
}

void game_renderer_impl::update_map_data(const_sub_region_range<tile_id> const sub_region, tile_map const& tmap) {
    auto dst_it = sub_region_iterator<data_t>(sub_region.first, tile_data.data());

    for (auto it = sub_region.first; it != sub_region.second; ++it, ++dst_it) {
        auto& dst = *dst_it;

        auto const tex_rect = tmap.index_to_rect(id_to_index(tmap, *it));

        dst.tex_coord.x = offset_type_x<uint16_t> {tex_rect.x0};
        dst.tex_coord.y = offset_type_y<uint16_t> {tex_rect.y0};
    }
}

void game_renderer_impl::update_map_data(level const& lvl, tile_map const& tmap) {
    auto const bounds = lvl.bounds();
    auto const bounds_size = static_cast<size_t>(std::max(0, bounds.area()));

    if (tile_data.size() < bounds_size) {
        tile_data.resize(bounds_size);
    }

    auto const region_color = [](uint32_t i) noexcept {
        ++i;
        return 0xFFu << 24 | (i * 11u) << 16 | (i * 23u) << 8 | (i * 37u);
    };

    auto const tw = value_cast(tmap.tile_width());
    auto const th = value_cast(tmap.tile_height());

    auto const ids_range        = lvl.tile_ids(bounds);
    auto const region_ids_range = lvl.region_ids(bounds);

    auto dst = sub_region_iterator<data_t> {ids_range.first, tile_data.data()};
    auto it0 = ids_range.first;
    auto it1 = region_ids_range.first;

    for ( ; it0 != ids_range.second; ++it0, ++it1, ++dst) {
        auto const tid = *it0;
        auto const rid = *it1;

        dst->position.x = offset_type_x<uint16_t> {dst.x() * tw};
        dst->position.y = offset_type_y<uint16_t> {dst.y() * th};

        dst->color = (tid == tile_id::empty)
          ? region_color(rid)
          : 0xFF0000FFu;

        auto const tex_rect = tmap.index_to_rect(id_to_index(tmap, tid));

        dst->tex_coord.x = offset_type_x<uint16_t> {tex_rect.x0};
        dst->tex_coord.y = offset_type_y<uint16_t> {tex_rect.y0};
    }
}

void game_renderer_impl::update_entity_data(level const& lvl, tile_map const& tmap) {
    auto const& epos = lvl.entity_positions();
    auto const& eids = lvl.entity_ids();

    BK_ASSERT(epos.size() == eids.size());

    auto const tw = value_cast(tmap.tile_width());
    auto const th = value_cast(tmap.tile_height());

    entity_data.clear();
    entity_data.reserve(epos.size());

    size_t i = 0;
    std::transform(begin(epos), end(epos), back_inserter(entity_data)
      , [&](point2<uint16_t> const p) noexcept {
            auto const tex_rect = tmap.index_to_rect(id_to_index(tmap, eids[i++]));

            auto const px = static_cast<uint16_t>(value_cast(p.x) * tw);
            auto const py = static_cast<uint16_t>(value_cast(p.y) * th);
            auto const tx = static_cast<uint16_t>(tex_rect.x0);
            auto const ty = static_cast<uint16_t>(tex_rect.y0);

            return data_t {point2<uint16_t> {px, py}
                         , point2<uint16_t> {tx, ty}
                         , 0xFFu};
        });
}

void game_renderer_impl::update_tool_tip_text(std::string text) {
    tool_tip_.layout(trender_, std::move(text));
}

void game_renderer_impl::update_tool_tip_visible(bool const show) noexcept {
    tool_tip_.visible(show);
}

void game_renderer_impl::update_tool_tip_position(point2i const p) noexcept {
    tool_tip_.move_to(value_cast(p.x), value_cast(p.y));
}

void game_renderer_impl::render(duration_t const delta, view const& v
  , tile_map const& tmap_base
  , tile_map const& tmap_entities
  , tile_map const& tmap_items
) const noexcept {
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
    render_text_(tool_tip_);

    //
    // message log window
    //
    render_message_log_();

    os_.render_present();
}

void game_renderer_impl::render_text_(text_layout const& text) const noexcept {
    if (!text.is_visible()) {
        return;
    }

    text.update(trender_);

    auto const& glyph_data = text.data();

    auto const p  = text.position();
    auto const tx = value_cast<float>(p.x);
    auto const ty = value_cast<float>(p.y);

    os_.render_set_transform(1.0f, 1.0f, tx, ty);

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

    std::for_each(log_window.visible_begin(), log_window.visible_end()
      , [&](text_layout const& line) { render_text_(line); });
}

} //namespace boken
