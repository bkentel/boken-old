#pragma once

#include "math_types.hpp"

#include <string>
#include <memory>

namespace boken { class text_renderer; }
namespace boken { class text_layout; }

namespace boken {

//!
class message_log {
public:
    virtual ~message_log();

    virtual void print(std::string msg) = 0;
    virtual void println(std::string msg) = 0;

    virtual recti32 bounds() const noexcept = 0;
    virtual recti32 client_bounds() const noexcept = 0;

    virtual int visible_size() const noexcept = 0;

    using ref = std::reference_wrapper<text_layout const>;

    virtual ref const* visible_begin() const noexcept = 0;
    virtual ref const* visible_end() const noexcept = 0;
};

std::unique_ptr<message_log> make_message_log(text_renderer& trender);

} //namespace boken
