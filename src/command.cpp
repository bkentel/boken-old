#include "command.hpp"
#include "system.hpp" // kb_event
#include "unicode.hpp"

namespace boken {

command_translator::~command_translator() = default;

class command_translator_impl final : public command_translator {
public:
    command_translator_impl();
    void on_command(command_handler_t handler) final override;
    void translate(text_input_event const& event) const final override;
    void translate(kb_event const& event) const final override;
private:
    command_handler_t handler_;
};

command_translator_impl::command_translator_impl()
{
    handler_ = [](auto, auto) noexcept {};
}

void command_translator_impl::on_command(command_handler_t handler) {
    handler_ = std::move(handler);
}

void command_translator_impl::translate(text_input_event const& event) const {
    utf8_decoder_iterator it {event.text.data()};
    auto const cp = *it;

    switch (cp) {
    case ',' :
        handler_(command_type::get_all_items, 0);
        break;
    case '<' :
        handler_(command_type::move_up, 0);
        break;
    case '>' :
        handler_(command_type::move_down, 0);
        break;
    default:
        break;
    }
}

void command_translator_impl::translate(kb_event const& event) const {
    switch (event.keycode) {
    case kb_keycode::k_t :
        if (kb_modifiers {event.mods}.test(kb_modifiers::m_ctrl)) {
            handler_(command_type::debug_teleport_self, 0);
            return;
        }
        break;
    default:
        break;
    }

    switch (event.scancode) {
    case kb_scancode::k_right :
        handler_(command_type::move_e, 0);
        break;
    case kb_scancode::k_left :
        handler_(command_type::move_w, 0);
        break;
    case kb_scancode::k_down :
        handler_(command_type::move_s, 0);
        break;
    case kb_scancode::k_up :
        handler_(command_type::move_n, 0);
        break;
    case kb_scancode::k_kp_1 :
        handler_(command_type::move_sw, 0);
        break;
    case kb_scancode::k_kp_2 :
        handler_(command_type::move_s, 0);
        break;
    case kb_scancode::k_kp_3 :
        handler_(command_type::move_se, 0);
        break;
    case kb_scancode::k_kp_4 :
        handler_(command_type::move_w, 0);
        break;
    case kb_scancode::k_kp_5 :
        handler_(command_type::move_here, 0);
        break;
    case kb_scancode::k_kp_6 :
        handler_(command_type::move_e, 0);
        break;
    case kb_scancode::k_kp_7 :
        handler_(command_type::move_nw, 0);
        break;
    case kb_scancode::k_kp_8 :
        handler_(command_type::move_n, 0);
        break;
    case kb_scancode::k_kp_9 :
        handler_(command_type::move_ne, 0);
        break;
    case kb_scancode::k_home :
        handler_(command_type::reset_view, 0);
        break;
    case kb_scancode::k_escape :
        handler_(command_type::cancel, 0);
        break;
    case kb_scancode::k_f1:
        handler_(command_type::debug_toggle_regions, 0);
        break;
    default:
        break;
    }
}

std::unique_ptr<command_translator> make_command_translator() {
    return std::make_unique<command_translator_impl>();
}

template <>
command_type string_to_enum(string_view const str) noexcept {
    auto const hash = djb2_hash_32(str.data());

    #define BK_ENUM_MAPPING(x) case command_type::x : return command_type::x
    switch (static_cast<command_type>(hash)) {
        BK_ENUM_MAPPING(none);
        BK_ENUM_MAPPING(move_here);
        BK_ENUM_MAPPING(move_n);
        BK_ENUM_MAPPING(move_ne);
        BK_ENUM_MAPPING(move_e);
        BK_ENUM_MAPPING(move_se);
        BK_ENUM_MAPPING(move_s);
        BK_ENUM_MAPPING(move_sw);
        BK_ENUM_MAPPING(move_w);
        BK_ENUM_MAPPING(move_nw);
        BK_ENUM_MAPPING(move_up);
        BK_ENUM_MAPPING(move_down);
        BK_ENUM_MAPPING(get_all_items);
        BK_ENUM_MAPPING(reset_zoom);
        BK_ENUM_MAPPING(reset_view);
        BK_ENUM_MAPPING(cancel);
        BK_ENUM_MAPPING(debug_toggle_regions);
        BK_ENUM_MAPPING(debug_teleport_self);
        default:
            break;
    }
    #undef BK_ENUM_MAPPING

    return command_type::none;
}

string_view enum_to_string(command_type const id) noexcept {
    #define BK_ENUM_MAPPING(x) case command_type::x : return #x
    switch (id) {
        BK_ENUM_MAPPING(none);
        BK_ENUM_MAPPING(move_here);
        BK_ENUM_MAPPING(move_n);
        BK_ENUM_MAPPING(move_ne);
        BK_ENUM_MAPPING(move_e);
        BK_ENUM_MAPPING(move_se);
        BK_ENUM_MAPPING(move_s);
        BK_ENUM_MAPPING(move_sw);
        BK_ENUM_MAPPING(move_w);
        BK_ENUM_MAPPING(move_nw);
        BK_ENUM_MAPPING(move_up);
        BK_ENUM_MAPPING(move_down);
        BK_ENUM_MAPPING(get_all_items);
        BK_ENUM_MAPPING(reset_zoom);
        BK_ENUM_MAPPING(reset_view);
        BK_ENUM_MAPPING(cancel);
        BK_ENUM_MAPPING(debug_toggle_regions);
        BK_ENUM_MAPPING(debug_teleport_self);
        default:
            break;
    }
    #undef BK_ENUM_MAPPING

    return "invalid command_type";
}

} //namespace boken
