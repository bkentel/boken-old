#include "text.hpp"
#include "system.hpp"   // for system
#include "utility.hpp"  // for BK_OFFSETOF
#include "math.hpp"
#include <algorithm>    // for move, max, swap

namespace {
inline uint32_t next_code_point(char const*& p, char const* const last) noexcept {
    return p != last ? static_cast<uint8_t>(*p++) : 0u;
}
} //namespace anonymous

namespace boken {

//===------------------------------------------------------------------------===
//                             text_renderer
//===------------------------------------------------------------------------===

text_renderer::~text_renderer() = default;

class text_renderer_impl final : public text_renderer {
public:
    glyph_data_t load_metrics(uint32_t const cp_prev, uint32_t const cp) noexcept final override {
        return load_metrics(cp);
    }

    glyph_data_t load_metrics(uint32_t const cp) noexcept final override;

    int pixel_size() const noexcept final override { return 18; }
    int ascender()   const noexcept final override { return 18; }
    int descender()  const noexcept final override { return 0; }
    int line_gap()   const noexcept final override { return 18; }
};

text_renderer::glyph_data_t
text_renderer_impl::load_metrics(uint32_t const cp) noexcept {
    constexpr uint32_t tiles_x = 16u;
    //constexpr int16_t tiles_y = 16;
    constexpr sizei16x tile_w  = int16_t {18};
    constexpr sizei16y tile_h  = int16_t {18};

    auto const tex_offset = point2i16 {
        value_cast(tile_w * static_cast<int16_t>(cp % tiles_x))
      , value_cast(tile_h * static_cast<int16_t>(cp / tiles_x))};

    auto const tex_size = point2i16 {
        value_cast(tile_w), value_cast(tile_h)};

    auto const offset  = vec2i16 {};
    auto const advance = vec2i16 {tile_w, int16_t {0}};

    return {tex_offset, tex_size, offset, advance};
}

std::unique_ptr<text_renderer> make_text_renderer() {
    return std::make_unique<text_renderer_impl>();
}

//===------------------------------------------------------------------------===
//                             text_layout
//===------------------------------------------------------------------------===

text_layout::text_layout() noexcept
  : data_          {}
  , text_          {}
  , position_      {}
  , max_width_     {std::numeric_limits<int16_t>::max()}
  , max_height_    {std::numeric_limits<int16_t>::max()}
  , actual_width_  {}
  , actual_height_ {}
  , is_visible_    {false}
{
}

text_layout::text_layout(
    text_renderer& trender
  , std::string text
  , sizei16x const max_width
  , sizei16y const max_height
)
  : data_          {}
  , text_          {}
  , position_      {}
  , max_width_     {max_width}
  , max_height_    {max_height}
  , actual_width_  {}
  , actual_height_ {}
  , is_visible_    {true}
{
    layout(trender, std::move(text));
}

void text_layout::layout(text_renderer& trender, std::string text) {
    text_ = std::move(text);
    layout(trender);
}

void text_layout::layout(text_renderer& trender) {
    data_.clear();

    auto const line_gap = int32_t {trender.line_gap()};
    auto const max_w    = int32_t {value_cast(max_width_)};
    auto const max_h    = int32_t {value_cast(max_height_)};

    auto     it       = text_.data();
    auto     last     = text_.data() + text_.size();
    uint32_t prev_cp  = 0;
    int32_t  line_h   = 0;
    int32_t  x        = 0;
    int32_t  y        = 0;
    int32_t  actual_w = 0;

    auto const next_line = [&] {
        actual_w =  std::max(actual_w, x);
        y        += line_gap;

        if (y >= max_h) {
            return false;
        }
        x      =  0;
        line_h =  line_gap;

        return true;
    };

    while (it != last) {
        auto const cp = next_code_point(it, last);

        if (cp == '\n') {
            next_line();
            continue; // consume the newline
        }

        auto const metrics = trender.load_metrics(prev_cp, cp);
        auto const glyph_w = int32_t {value_cast(metrics.size.x)};
        auto const glyph_h = int32_t {value_cast(metrics.size.y)};

        if ((x + glyph_w > max_w) && !next_line()) {
            break;
        }

        data_.push_back(data_t {
            underlying_cast_unsafe<int16_t>(point2i32 {x, y})
          , metrics.texture
          , metrics.size
          , 0xFFFFFFFFu
        });

        line_h =  std::max(line_h, glyph_h);
        x      += glyph_w;
    }

    actual_w = std::max(actual_w, x);

    actual_width_  = size_type_x<int16_t> {static_cast<int16_t>(actual_w)};
    actual_height_ = size_type_y<int16_t> {static_cast<int16_t>(y + (x ? line_h : 0))};
}

void text_layout::update(text_renderer& trender) const noexcept {
    auto it   = text_.data();
    auto last = text_.data() + text_.size();

    size_t i = 0;
    while (it != last) {
        auto const cp = next_code_point(it, last);

        if (cp == '\n') {
            continue;
        }

        data_[i++].texture = trender.load_metrics(cp).texture;
    }
}

std::vector<text_layout::data_t> const& text_layout::data() const noexcept {
    return data_;
}

string_view text_layout::text() const noexcept {
    return {text_};
}

void text_layout::move_to(int const x, int const y) noexcept {
    position_ = point2<int16_t> {
        clamp_as<int16_t>(x)
      , clamp_as<int16_t>(y)};
}

point2i32 text_layout::position() const noexcept {
    return position_;
}

bool text_layout::is_visible() const noexcept {
    return is_visible_;
}

bool text_layout::visible(bool state) noexcept {
    return std::swap(is_visible_, state), state;
}

recti32 text_layout::extent() const noexcept {
    return {position_, actual_width_, actual_height_};
}

sizei32x text_layout::max_width() const noexcept {
    return max_width_;
}

sizei32y text_layout::max_height() const noexcept {
    return max_height_;
}

} //namespace boken
