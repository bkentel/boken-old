#include "render.hpp"
#include "level.hpp"
#include "math.hpp"
#include "system.hpp"
#include "tile.hpp"
#include "utility.hpp"

#include <bkassert/assert.hpp>

#include <vector>
#include <cstdint>

namespace boken {

game_renderer::~game_renderer() = default;

class game_renderer_impl final : public game_renderer {
public:
    game_renderer_impl(system& os)
      : os_ {os}
    {
    }

    void update_map_data(level const& lvl) final override;

    void render(duration_t delta, view const& v) const noexcept final override;
private:
    struct data_t {
        point2<uint16_t> position;
        point2<uint16_t> tex_coord;
        uint32_t         color;
    };

    std::vector<data_t> tile_data;
    std::vector<data_t> entity_data;

    tile_map base_tile_map;

    system& os_;
};

std::unique_ptr<game_renderer> make_game_renderer(system& os) {
    return std::make_unique<game_renderer_impl>(os);
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
    auto const  tile_pair      = lvl.tile_indicies(0);
    auto const& region_ids     = region_id_pair.first;
    auto const& tile_indicies  = tile_pair.first;
    auto const  tile_rect      = tile_pair.second;
    auto const  w              = tile_rect.width();

    auto const tw = value_cast(base_tile_map.tile_w);
    auto const th = value_cast(base_tile_map.tile_h);
    auto const tx = value_cast(base_tile_map.tiles_x);

    BK_ASSERT(region_id_pair.second == tile_pair.second);
    BK_ASSERT(tile_rect.x0 >= 0 && tile_rect.x1 >= tile_rect.x0);
    BK_ASSERT(tile_rect.y0 >= 0 && tile_rect.y1 >= tile_rect.y0);

    for_each_xy(tile_rect, [&](int const x, int const y) noexcept {
        auto const  i   = static_cast<size_t>(x + y * w);
        auto const& src = tile_indicies[i];
        auto&       dst = tile_data[i];

        dst.position.x = offset_type_x<uint16_t> {x * tw};
        dst.position.y = offset_type_y<uint16_t> {y * th};

        if (src == 11u + 13u * 16u) {
            dst.color = colors[region_ids[i]];
        } else {
            dst.color = 0xFF0000FFu;
        }

        dst.tex_coord.x = offset_type_x<uint16_t> {(src % tx) * tw};
        dst.tex_coord.y = offset_type_y<uint16_t> {(src / tx) * th};
    });
}

void game_renderer_impl::render(duration_t const delta, view const& v) const noexcept {
    os_.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);

    os_.render_background();

    os_.render_set_transform(v.scale_x, v.scale_y
                           , v.x_off,   v.y_off);

    os_.render_set_tile_size(value_cast(base_tile_map.tile_w)
                           , value_cast(base_tile_map.tile_h));

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
}

} //namespace boken
