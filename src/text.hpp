#pragma once

#include "math_types.hpp" // for basic_2_tuple, point2i
#include "config.hpp"     // for string_View

#include <memory>  // for unique_ptr
#include <string>  // for string
#include <vector>  // for vector
#include <cstdint> // for int16_t, uint32_t

namespace boken {

class text_renderer {
public:
    struct glyph_data_t {
        point2i16 texture;
        point2i16 size;
        vec2i16   offset;
        vec2i16   advance;
    };

    virtual ~text_renderer();

    virtual glyph_data_t load_metrics(uint32_t cp_prev, uint32_t cp) noexcept = 0;
    virtual glyph_data_t load_metrics(uint32_t cp) noexcept = 0;

    virtual int pixel_size() const noexcept = 0;
    virtual int ascender()   const noexcept = 0;
    virtual int descender()  const noexcept = 0;
    virtual int line_gap()   const noexcept = 0;
};

std::unique_ptr<text_renderer> make_text_renderer();

class text_layout {
public:
    struct data_t {
        point2i16 position;
        point2i16 texture;
        point2i16 size;
        uint32_t  color;
        uint32_t  codepoint;
    };

    static constexpr sizei16x none_x() noexcept { return std::numeric_limits<int16_t>::max(); }
    static constexpr sizei16y none_y() noexcept { return std::numeric_limits<int16_t>::max(); }

    text_layout() noexcept;

    text_layout(
        text_renderer& trender
      , std::string    text
      , sizei16x       max_width  = none_x()
      , sizei16y       max_height = none_y()
    );

    // replacing the current text with @p text, layout the text.
    void layout(text_renderer& trender, std::string text);

    // using the current dimensions and text, layout the text.
    void layout(text_renderer& trender);

    void move_to(int x, int y) noexcept;
    point2i32 position() const noexcept;

    bool is_visible() const noexcept;
    bool visible(bool state) noexcept;

    recti32 extent() const noexcept;
    sizei32x max_width() const noexcept;
    sizei32y max_height() const noexcept;

    void set_max_width(sizei32x w) noexcept;

    // ensure all required glyphs are still cached at the same locations
    void update(text_renderer& trender) const noexcept;

    std::vector<data_t> const& data() const noexcept;

    string_view text() const noexcept;
private:
    // glyph texture locations can change
    std::vector<data_t> mutable data_;

    std::string text_;
    point2i16   position_;
    sizei16x    max_width_;
    sizei16y    max_height_;
    sizei16x    actual_width_;
    sizei16y    actual_height_;
    bool        is_visible_;
};

} // namespace boken
