#pragma once

#include "math.hpp" // point
#include <string>
#include <vector>
#include <limits>
#include <cstdint>

namespace boken {

class system;

class text_renderer {
public:
    struct glyph_data_t {
        point2<int16_t> texture;
        point2<int16_t> size;
        point2<int16_t> offset;
        point2<int16_t> advance;
    };

    glyph_data_t load_metrics(uint32_t const cp_prev, uint32_t const cp) {
        return load_metrics(cp);
    }

    glyph_data_t load_metrics(uint32_t const cp) {
        constexpr int16_t tiles_x = 16;
        constexpr int16_t tiles_y = 16;
        constexpr int16_t tile_w  = 18;
        constexpr int16_t tile_h  = 18;

        auto const tx = static_cast<int16_t>((cp % tiles_x) * tile_w);
        auto const ty = static_cast<int16_t>((cp / tiles_x) * tile_h);

        return {
            {    tx,     ty}
          , {tile_w, tile_h}
          , {     0,      0}
          , {tile_w,      0}
        };
    }
};

class text_layout {
public:
    text_layout() = default;

    text_layout(text_renderer& trender, std::string text);

    void layout(text_renderer& trender, std::string text);

    void layout(text_renderer& trender);

    void render(system& os, text_renderer& trender);

    void move_to(int x, int y);
private:
    struct data_t {
        point2<int16_t> position;
        point2<int16_t> texture;
        point2<int16_t> size;
        uint32_t        color;
    };

    std::string          text_       {};
    std::vector<data_t>  data_       {};
    point2<int>          position_   {0, 0};
    size_type_x<int16_t> max_width_  {std::numeric_limits<int16_t>::max()};
    size_type_y<int16_t> max_height_ {std::numeric_limits<int16_t>::max()};
};

} // namespace boken
