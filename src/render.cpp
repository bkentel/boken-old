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

    tile_map const& base_tile_map() const noexcept final override {
        return base_tile_map_;
    }

    void update_map_data(level const& lvl) final override;
    void update_map_data(level const& lvl, recti area) final override;
    void update_entity_data(level const& lvl) final override;

    void update_tool_tip_text(std::string text) final override;
    void update_tool_tip_visible(bool show) noexcept final override;
    void update_tool_tip_position(point2i p) noexcept final override;

    void set_message_window(message_log const* const window) noexcept final override {
        message_log_ = window;
    }

    void render(duration_t delta, view const& v) const noexcept final override;
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

    tile_map base_tile_map_;

    text_layout tool_tip_;
    message_log const* message_log_ {};
};

std::unique_ptr<game_renderer> make_game_renderer(system& os, text_renderer& trender) {
    return std::make_unique<game_renderer_impl>(os, trender);
}

void game_renderer_impl::update_map_data(level const& lvl, recti area) {

}

void game_renderer_impl::update_map_data(level const& lvl) {
    {
        auto const size = value_cast(lvl.width()) * value_cast(lvl.height());
        if (tile_data.size() < size) {
            tile_data.resize(size);
        }
    }

    std::vector<uint32_t> colors;
    std::generate_n(back_inserter(colors), lvl.region_count()
      , [&] {
            auto const s = static_cast<uint32_t>(colors.size() + 1);
            return 0xFF << 24 | (s * 11) << 16 | (s * 23) << 8 | (s * 37);
        });

    auto const  region_id_pair = lvl.region_ids(0);
    auto const  tile_pair      = lvl.tile_ids(0);
    auto const& region_ids     = region_id_pair.first;
    auto const& tile_ids       = tile_pair.first;
    auto const  tile_rect      = tile_pair.second;
    auto const  w              = tile_rect.width();

    auto const tw = value_cast(base_tile_map_.tile_w);
    auto const th = value_cast(base_tile_map_.tile_h);
    auto const tx = value_cast(base_tile_map_.tiles_x);

    BK_ASSERT(region_id_pair.second == tile_pair.second);
    BK_ASSERT(tile_rect.x0 >= 0 && tile_rect.x1 >= tile_rect.x0);
    BK_ASSERT(tile_rect.y0 >= 0 && tile_rect.y1 >= tile_rect.y0);

    for_each_xy(tile_rect, [&](int const x, int const y) noexcept {
        auto const  i   = static_cast<size_t>(x + y * w);
        auto const& src = tile_ids[i];
        auto&       dst = tile_data[i];

        dst.position.x = offset_type_x<uint16_t> {x * tw};
        dst.position.y = offset_type_y<uint16_t> {y * th};

        if (src == tile_id::empty) {
            dst.color = colors[region_ids[i]];
        } else {
            dst.color = 0xFF0000FFu;
        }

        auto const tex_rect = base_tile_map_.index_to_rect(
            base_tile_map_.id_to_index(tile_ids[i]));

        dst.tex_coord.x = offset_type_x<uint16_t> {tex_rect.x0};
        dst.tex_coord.y = offset_type_y<uint16_t> {tex_rect.y0};
    });
}

void game_renderer_impl::update_entity_data(level const& lvl) {
    auto const& epos = lvl.entity_positions();
    auto const& eids = lvl.entity_ids();

    BK_ASSERT(epos.size() == eids.size());

    auto const tw = value_cast(base_tile_map_.tile_w);
    auto const th = value_cast(base_tile_map_.tile_h);

    entity_data.clear();
    entity_data.reserve(epos.size());

    std::transform(begin(epos), end(epos), back_inserter(entity_data)
      , [&](point2<uint16_t> const p) noexcept {
            auto const px = static_cast<uint16_t>(value_cast(p.x) * tw);
            auto const py = static_cast<uint16_t>(value_cast(p.y) * th);
            auto const tx = static_cast<uint16_t>(1 * tw);
            auto const ty = static_cast<uint16_t>(0 * th);

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

void game_renderer_impl::render(duration_t const delta, view const& v) const noexcept {
    os_.render_clear();

    os_.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);

    os_.render_background();

    os_.render_set_transform(v.scale_x, v.scale_y
                           , v.x_off,   v.y_off);

    os_.render_set_tile_size(value_cast(base_tile_map_.tile_w)
                           , value_cast(base_tile_map_.tile_h));

    //
    // Map tiles
    //
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
    os_.render_set_data(render_data_type::position
      , read_only_pointer_t {entity_data, BK_OFFSETOF(data_t, position)});
    os_.render_set_data(render_data_type::texture
      , read_only_pointer_t {entity_data, BK_OFFSETOF(data_t, tex_coord)});
    os_.render_set_data(render_data_type::color
      , read_only_pointer_t {entity_data, BK_OFFSETOF(data_t, color)});
    os_.render_data_n(entity_data.size());

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
