#pragma once
#include "math.hpp"   // for basic_2_tuple, point2i
#include "types.hpp"  // for tagged_integral_value
#include "config.hpp" //string_View

#include <memory>     // for unique_ptr
#include <string>     // for string
#include <vector>     // for vector

#include <cstdint>    // for int16_t, uint32_t

namespace boken {

class text_renderer {
public:
    struct glyph_data_t {
        point2<int16_t> texture;
        point2<int16_t> size;
        point2<int16_t> offset;
        point2<int16_t> advance;
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
        point2<int16_t> position;
        point2<int16_t> texture;
        point2<int16_t> size;
        uint32_t        color;
    };

    text_layout() noexcept;

    text_layout(
        text_renderer& trender
      , std::string    text
      , int16_t        max_width  = std::numeric_limits<int16_t>::max()
      , int16_t        max_height = std::numeric_limits<int16_t>::max()
    );

    void layout(text_renderer& trender, std::string text);

    void layout(text_renderer& trender);

    void move_to(int x, int y) noexcept;
    point2i position() const noexcept;

    bool is_visible() const noexcept;
    bool visible(bool state) noexcept;

    recti extent() const noexcept;
    sizeix max_width() const noexcept;
    sizeiy max_height() const noexcept;

    // ensure all required glyphs are still cached at the same locations
    void update(text_renderer& trender) const noexcept;

    std::vector<data_t> const& data() const noexcept;

    string_view text() const noexcept;
private:
    // glyph texture locations can change
    std::vector<data_t> mutable data_;

    std::string          text_;
    point2<int16_t>      position_;
    size_type_x<int16_t> max_width_;
    size_type_y<int16_t> max_height_;
    size_type_x<int16_t> actual_width_;
    size_type_y<int16_t> actual_height_;
    bool                 is_visible_;
};

} // namespace boken
