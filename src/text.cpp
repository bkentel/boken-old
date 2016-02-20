#include "text.hpp"
#include "system.hpp"
#include "utility.hpp"

namespace boken {

text_layout::text_layout(text_renderer& trender, std::string text)
  : text_ {std::move(text)}
{
    layout(trender);
}

void text_layout::layout(text_renderer& trender, std::string text) {
    text_ = std::move(text);
    layout(trender);
}

void text_layout::layout(text_renderer& trender) {
    data_.clear();

    auto const next_code_point = [&](char const*& p, char const* const last) -> uint32_t {
        return p != last ? *p++ : 0;
    };

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
            {static_cast<int16_t>(x + value_cast(position_.x))
           , static_cast<int16_t>(y + value_cast(position_.y))}
           , metrics.texture
           , metrics.size
           , 0xFFFFFFFFu
        });

        line_h =  std::max<int32_t>(line_h, value_cast(metrics.size.y));
        x      += value_cast(metrics.size.x);
    }
}

void text_layout::render(system& os, text_renderer& trender) const {
    if (!is_visible_) {
        return;
    }

    auto const next_code_point = [&](char const*& p, char const* const last) -> uint32_t {
        return p != last ? *p++ : 0;
    };

    auto it   = text_.data();
    auto last = text_.data() + text_.size();

    for (size_t i = 0; it != last; ++i) {
        data_[i].texture = trender.load_metrics(next_code_point(it, last)).texture;
    }

    os.render_set_data(render_data_type::position, read_only_pointer_t {
        data_, BK_OFFSETOF(data_t, position)});
    os.render_set_data(render_data_type::texture, read_only_pointer_t {
        data_, BK_OFFSETOF(data_t, texture)});
    os.render_set_data(render_data_type::color, read_only_pointer_t {
        data_, BK_OFFSETOF(data_t, color)});

    os.render_data_n(data_.size());
}

void text_layout::move_to(int const x, int const y) noexcept {
    auto const u = point2i {x, y} - position_;
    auto const v = u.cast_to<int16_t>();

    for (auto& glyph : data_) {
        glyph.position += v;
    }

    position_ = point2i {x, y};
}

bool text_layout::is_visible() const noexcept {
    return is_visible_;
}

bool text_layout::visible(bool state) noexcept {
    return std::swap(is_visible_, state), state;
}

} //namespace boken
