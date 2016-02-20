#pragma once

#include "circular_buffer.hpp"
#include "text.hpp"
#include <string>

namespace boken {

class text_renderer;

//!
class message_log {
public:
    explicit message_log(text_renderer& trender)
      : trender_ {trender}
    {
    }

    void print(std::string msg) {
    }

    void println(std::string msg) {
        visible_lines_.push(text_layout {trender_, msg});
        messages_.push(std::move(msg));
    }

    auto visible_begin() const noexcept {
        return visible_lines_.begin();
    }

    auto visible_end() const noexcept {
        return visible_lines_.end();
    }

    auto max_visible() const noexcept {
        return visible_lines_.size();
    }
private:
    text_renderer& trender_;

    simple_circular_buffer<text_layout> visible_lines_ {10};
    simple_circular_buffer<std::string> messages_      {50};
};

} //namespace boken
