#include "text.hpp"
#include "system.hpp"   // for system
#include "utility.hpp"  // for BK_OFFSETOF
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
    constexpr int16_t tiles_x = 16;
    //constexpr int16_t tiles_y = 16;
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

std::unique_ptr<text_renderer> make_text_renderer() {
    return std::make_unique<text_renderer_impl>();
}

//===------------------------------------------------------------------------===
//                             text_layout
//===------------------------------------------------------------------------===

text_layout::text_layout()
  : data_       {}
  , text_       {}
  , position_   {0, 0}
  , max_width_  {std::numeric_limits<int16_t>::max()}
  , max_height_ {std::numeric_limits<int16_t>::max()}
  , is_visible_ {false}
{
}

text_layout::text_layout(text_renderer& trender, std::string text)
  : text_layout {}
{
    is_visible_ = true;
    layout(trender, std::move(text));
}

void text_layout::layout(text_renderer& trender, std::string text) {
    text_ = std::move(text);
    layout(trender);
}

void text_layout::layout(text_renderer& trender) {
    data_.clear();

    auto     it      = text_.data();
    auto     last    = text_.data() + text_.size();
    uint32_t prev_cp = 0;
    int32_t  line_h  = 0;
    int32_t  x       = 0;
    int32_t  y       = 0;

    while (it != last) {
        auto const cp = next_code_point(it, last);
        auto const metrics = trender.load_metrics(prev_cp, cp);

        if (x + value_cast(metrics.size.x) > value_cast(max_width_)) {
            y      += line_h;
            x      =  0;
            line_h =  0;
        }

        data_.push_back(data_t {
            point2i {x, y}.cast_to<int16_t>()
          , metrics.texture
          , metrics.size
          , 0xFFFFFFFFu
        });

        line_h =  std::max<int32_t>(line_h, value_cast(metrics.size.y));
        x      += value_cast(metrics.size.x);
    }
}

void text_layout::update(text_renderer& trender) const noexcept {
    auto it   = text_.data();
    auto last = text_.data() + text_.size();

    for (size_t i = 0; it != last; ++i) {
        data_[i].texture = trender.load_metrics(next_code_point(it, last)).texture;
    }
}

std::vector<text_layout::data_t> const& text_layout::data() const noexcept {
    return data_;
}

void text_layout::move_to(int const x, int const y) noexcept {
    position_ = point2<int16_t> {
        clamp_as<int16_t>(x)
      , clamp_as<int16_t>(y)};
}

point2i text_layout::position() const noexcept {
    return position_.cast_to<int32_t>();
}

bool text_layout::is_visible() const noexcept {
    return is_visible_;
}

bool text_layout::visible(bool state) noexcept {
    return std::swap(is_visible_, state), state;
}

} //namespace boken
