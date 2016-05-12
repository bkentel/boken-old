#include "text.hpp"
#include "system.hpp"   // for system
#include "utility.hpp"  // for BK_OFFSETOF
#include "math.hpp"

#include <algorithm>    // for move, max, swap
#include <array>

namespace boken {

namespace {

template <typename FwdIt>
uint32_t next_code_point(FwdIt& it, FwdIt const last) noexcept {
    if (it == last) {
        return 0u;
    }

    auto result = *it;
    ++it;

    return static_cast<uint8_t>(result);
}

} //namespace anonymous

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

    enum class state_t {
        read, read_escape, read_markup, proccess, skip, stop, escape, markup
    };

    int16_t const line_gap = static_cast<int16_t>(trender.line_gap()); // gap between successive lines
    int16_t const max_w    = value_cast(max_width_);
    int16_t const max_h    = value_cast(max_height_);

    int32_t x        = 0; // current x position
    int32_t y        = 0; // current y position
    int32_t line_h   = 0; // real line height
    int32_t actual_w = 0; // the width of the longest line

    size_t line_start_pos = 0; // index of the element where this line started
    size_t line_break_pos = 0; // index of the last candidate for a line break

    //                                  AA'BB'GG'RR
    constexpr uint32_t c_light_gray = 0xFF'DD'DD'DDu;
    constexpr uint32_t c_bright_red = 0xFF'00'00'FFu;

    uint32_t color = c_light_gray;

    auto       markup_tag = std::array<char, 32> {};
    auto       markup_it  = begin(markup_tag);
    auto const markup_end = end(markup_tag);

    auto const break_line = [&](uint32_t const cp) noexcept {
        auto const next_line = [&]() noexcept {
            actual_w  = std::max(actual_w, x);
            x         = 0;

            if (y + line_gap > max_h) {
                return false;
            }

            y += line_gap;
            return true;
        };

        if (!next_line()) {
            return state_t::stop;
        }

        if (cp == '\n') {
            return state_t::skip;
        } else if (line_break_pos == line_start_pos) {
            return state_t::proccess;
        } else if (line_break_pos + 1 >= data_.size()) {
            return state_t::proccess;
        }

        auto const first = begin(data_) + static_cast<ptrdiff_t>(line_break_pos + 1);
        auto const last  = end(data_);

        auto const dx = static_cast<int16_t>(-value_cast(first->position.x));
        auto const dy = line_gap;
        auto const v  = vec2i16 {dx, dy};

        std::for_each(first, last, [&](data_t& glyph) noexcept {
            glyph.position += v;
        });

        auto const& last_glyph = data_.back();
        x = value_cast(last_glyph.position.x) + value_cast(last_glyph.size.x);

        return state_t::proccess;
    };

    auto const process_escape_seq = [&](uint32_t const cp) noexcept {
        return state_t::proccess;
    };

    auto const apply_markup = [&] {
        // <.*>
        auto const size = std::distance(begin(markup_tag), markup_it);
        BK_ASSERT(size >= 2 && markup_tag[0] == '<');

        switch (markup_tag[1]) {
        case '/' :
            switch (markup_tag[2]) {
            case 'c' :
                color = c_light_gray;
                break;
            default:
                break;
            }

            break;
        case 'c' :
            switch (markup_tag[2]) {
            case 'r' :
                color = c_bright_red;
                break;
            default:
                break;
            }

            break;
        default:
            break;
        }
    };

    auto const process_markup = [&](uint32_t const cp) noexcept {
        if (markup_it == markup_end) {
            BK_ASSERT(false);
            markup_it = begin(markup_tag);
            return state_t::read;
        }

        if (cp == '<') {
            markup_it = begin(markup_tag);
        }

        *markup_it = static_cast<char>(cp & 0x7Fu);
        ++markup_it;

        if (cp == '>') {
            apply_markup();
            return state_t::read;
        }

        return state_t::read_markup;
    };

    auto const process_cp = [&](uint32_t const cp) noexcept {
        switch (cp) {
        case '\\':
            return state_t::escape;
        case '\n':
            return break_line(cp);
        case '<' :
            return state_t::markup;
        default :
            break;
        }

        return state_t::proccess;
    };

    auto const load_glyph = [&](uint32_t const prev_cp, uint32_t const cp) {
        auto    const m       = trender.load_metrics(prev_cp, cp);
        int32_t const glyph_w = value_cast(m.size.x);
        int32_t const glyph_h = value_cast(m.size.y);

        if (x + glyph_w > max_w) {
            auto const result = break_line(cp);
            if (result != state_t::proccess) {
                return result;
            }

            line_start_pos = data_.size();
        }

        if (cp == ' ') {
            line_break_pos = data_.size();
        }

        auto const p = point2i16 {
            static_cast<int16_t>(x)
          , static_cast<int16_t>(y)
        };

        data_.push_back({p, m.texture, m.size, color, cp});

        x      += glyph_w;
        line_h =  std::max(line_h, glyph_h);

        return state_t::read;
    };

    // for each codepoint ...
    uint32_t prev_cp = 0;
    uint32_t cp      = 0;

    auto       it   = text_.data();
    auto const last = text_.data() + static_cast<ptrdiff_t>(text_.size());

    auto const read_next = [&]() noexcept {
        if (it == last) {
            return false;
        }

        prev_cp = cp;
        cp = next_code_point(it, last);

        return true;
    };

    for (auto state = state_t::read; ; ) {
        switch (state) {
        case state_t::read_escape :
            state = read_next()
              ? process_escape_seq(cp)
              : state_t::stop;
            break;
        case state_t::read_markup :
            state = read_next()
              ? process_markup(cp)
              : state_t::stop;
            break;
        case state_t::read :
            state = read_next()
              ? process_cp(cp)
              : state_t::stop;
            break;
        case state_t::proccess :
            state = load_glyph(prev_cp, cp);
            break;
        case state_t::skip :
            state = state_t::read;
            break;
        case state_t::escape :
            state = process_escape_seq(cp);
            break;
        case state_t::markup :
            state = process_markup(cp);
            break;
        case state_t::stop :
            actual_width_  = static_cast<int16_t>(std::max(actual_w, x));
            actual_height_ = static_cast<int16_t>(y + (x ? line_h : 0));
            return;
        default :
            BK_ASSERT(false);
            break;
        }
    }
}

void text_layout::update(text_renderer& trender) const noexcept {
    for (auto& glyph : data_) {
        glyph.texture = trender.load_metrics(glyph.codepoint).texture;
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
