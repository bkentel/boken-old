#include "message_log.hpp"
#include "circular_buffer.hpp"
#include "text.hpp"

#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdint>
#include <cstddef>

namespace boken {

message_log::~message_log() = default;

class message_log_impl final : public message_log {
public:
    explicit message_log_impl(text_renderer& trender)
      : trender_ {trender}
    {
    }

    void print(std::string msg) final override {

    }

    void update_buffer_() {
        buffer_.clear();

        auto const first = begin(visible_lines_);
        auto const last  = end(visible_lines_);
        std::transform(first, last, back_inserter(buffer_)
          , [](auto const& l) noexcept { return std::cref(l); });
    }

    void println(std::string msg) final override;

    recti32 bounds() const noexcept final override {
        return bounds_;
    }

    recti32 client_bounds() const noexcept final override {
        return client_bounds_;
    }

    int visible_size() const noexcept final override {
        return static_cast<int>(visible_lines_.size());
    }

    ref const* visible_begin() const noexcept final override {
        return buffer_.data();
    }

    ref const* visible_end() const noexcept final override {
        return buffer_.data() + static_cast<ptrdiff_t>(buffer_.size());
    }

private:
    text_renderer& trender_;
    recti32        bounds_ {point2i32 {}, sizei32x {500}, sizei32y {200}};
    recti32        client_bounds_ {};

    std::vector<ref>                    buffer_;
    simple_circular_buffer<text_layout> visible_lines_ {10};
    simple_circular_buffer<std::string> messages_      {50};
};

std::unique_ptr<message_log> make_message_log(text_renderer& trender) {
    return std::make_unique<message_log_impl>(trender);
}

void message_log_impl::println(std::string msg) {
    auto const bounds_r = bounds();
    auto const max_w    = underlying_cast_unsafe<int16_t>(bounds_r.width());
    auto const p        = bounds_r.top_left();

    auto actual_w = int32_t {0};
    auto actual_h = int32_t {0};

    auto x = value_cast(p.x);
    auto y = value_cast(p.y);

    visible_lines_.push(text_layout {trender_, msg, max_w});
    messages_.push(std::move(msg));

    for (auto& line : visible_lines_) {
        auto const r = line.extent();
        auto const h = value_cast(r.height());
        auto const w = value_cast(r.width());

        line.move_to(x, y);
        y += h;

        actual_w = std::max(actual_w, w);
    }

    actual_h = y;

    client_bounds_ = recti32 {
        p
      , sizei32x {actual_w}
      , sizei32y {actual_h}};

    update_buffer_();
}

} //namespace boken
