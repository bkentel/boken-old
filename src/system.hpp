#pragma once

#include "math_types.hpp"
#include "system_input.hpp"

#include <memory>
#include <functional>

#include <cstdint>

namespace boken {

class system {
public:
    using on_resize_handler       = std::function<void (int32_t, int32_t)>;
    using on_request_quit_handler = std::function<bool ()>;
    using on_key_handler          = std::function<void (kb_event, kb_modifiers)>;
    using on_mouse_move_handler   = std::function<void (mouse_event, kb_modifiers)>;
    using on_mouse_button_handler = std::function<void (mouse_event, kb_modifiers)>;
    using on_mouse_wheel_handler  = std::function<void (int32_t, int32_t, kb_modifiers)>;
    using on_text_input_handler   = std::function<void (text_input_event)>;

    virtual ~system();

    virtual void on_resize(on_resize_handler handler) = 0;
    virtual void on_request_quit(on_request_quit_handler handler) = 0;
    virtual void on_key(on_key_handler handler) = 0;
    virtual void on_mouse_move(on_mouse_move_handler handler) = 0;
    virtual void on_mouse_button(on_mouse_button_handler handler) = 0;
    virtual void on_mouse_wheel(on_mouse_wheel_handler handler) = 0;
    virtual void on_text_input(on_text_input_handler handler) = 0;

    virtual bool is_running() = 0;
    virtual int32_t do_events() = 0;

    virtual recti32 get_client_rect() const = 0;
};

std::unique_ptr<system> make_system();

} //namespace boken
