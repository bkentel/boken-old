#include "command.hpp"
#include "system_input.hpp" // kb_event
#include "unicode.hpp"

namespace boken {

command_translator::~command_translator() = default;

class command_translator_impl final : public command_translator {
public:
    command_translator_impl();
    void on_command(command_handler_t handler) final override;
    void translate(text_input_event const& event) const final override;
    void translate(kb_event const& event, kb_modifiers const& kmods) const final override;
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
    case 'i':
        handler_(command_type::toggle_show_inventory, 0);
        break;
    case 'd':
        handler_(command_type::drop_one, 0);
        break;
    case 'D':
        handler_(command_type::drop_some, 0);
        break;
    case 'g':
        handler_(command_type::get_items, 0);
        break;
    case 'o':
        handler_(command_type::open, 0);
        break;
    case 'v':
        handler_(command_type::view, 0);
        break;
    default:
        break;
    }
}

void command_translator_impl::translate(kb_event const& event, kb_modifiers const& kmods) const {
    using ct = command_type;
    using km = kb_modifiers;

    if (!event.went_down) {
        return;
    }

    auto const event_kmods = kb_modifiers {event.mods};

    switch (event.keycode) {
    case kb_keycode::k_d :
        if (event_kmods.exclusive_any(kb_mod::ctrl)) {
            handler_(command_type::alt_drop_some, 0);
            return;
        }
        break;
    case kb_keycode::k_g :
        if (event_kmods.exclusive_any(kb_mod::ctrl)) {
            handler_(command_type::alt_get_items, 0);
            return;
        }
        break;
    case kb_keycode::k_o :
        if (event_kmods.exclusive_any(kb_mod::ctrl)) {
            handler_(command_type::alt_open, 0);
            return;
        }
        break;
    case kb_keycode::k_t :
        if (event_kmods.exclusive_any(kb_mod::ctrl)) {
            handler_(command_type::debug_teleport_self, 0);
            return;
        }
        break;
    default:
        break;
    }

    switch (event.scancode) {
    case kb_scancode::k_space:
        if (kmods.none()) {
            handler_(ct::toggle, 0);
        }
        break;
    case kb_scancode::k_return   : BK_ATTRIBUTE_FALLTHROUGH;
    case kb_scancode::k_kp_enter :
        if (kmods.none()) {
            handler_(ct::confirm, 0);
        }
        break;
    case kb_scancode::k_right : BK_ATTRIBUTE_FALLTHROUGH;
    case kb_scancode::k_kp_6  :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_e : ct::move_e, 0);
        break;
    case kb_scancode::k_left : BK_ATTRIBUTE_FALLTHROUGH;
    case kb_scancode::k_kp_4 :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_w : ct::move_w, 0);
        break;
    case kb_scancode::k_down : BK_ATTRIBUTE_FALLTHROUGH;
    case kb_scancode::k_kp_2 :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_s : ct::move_s, 0);
        break;
    case kb_scancode::k_up   : BK_ATTRIBUTE_FALLTHROUGH;
    case kb_scancode::k_kp_8 :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_n : ct::move_n, 0);
        break;
    case kb_scancode::k_kp_1 :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_sw : ct::move_sw, 0);
        break;
    case kb_scancode::k_kp_3 :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_se : ct::move_se, 0);
        break;
    case kb_scancode::k_kp_5 :
        handler_(command_type::move_here, 0);
        break;
    case kb_scancode::k_kp_7 :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_nw : ct::move_nw, 0);
        break;
    case kb_scancode::k_kp_9 :
        handler_(kmods.exclusive_any(kb_mod::shift) ? ct::run_ne : ct::move_ne, 0);
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
        BK_ENUM_MAPPING(run_n);
        BK_ENUM_MAPPING(run_ne);
        BK_ENUM_MAPPING(run_e);
        BK_ENUM_MAPPING(run_se);
        BK_ENUM_MAPPING(run_s);
        BK_ENUM_MAPPING(run_sw);
        BK_ENUM_MAPPING(run_w);
        BK_ENUM_MAPPING(run_nw);
        BK_ENUM_MAPPING(move_up);
        BK_ENUM_MAPPING(move_down);
        BK_ENUM_MAPPING(get_all_items);
        BK_ENUM_MAPPING(get_items);
        BK_ENUM_MAPPING(reset_zoom);
        BK_ENUM_MAPPING(reset_view);
        BK_ENUM_MAPPING(cancel);
        BK_ENUM_MAPPING(confirm);
        BK_ENUM_MAPPING(toggle);
        BK_ENUM_MAPPING(drop_one);
        BK_ENUM_MAPPING(drop_some);
        BK_ENUM_MAPPING(open);
        BK_ENUM_MAPPING(view);
        BK_ENUM_MAPPING(alt_get_items);
        BK_ENUM_MAPPING(alt_drop_some);
        BK_ENUM_MAPPING(alt_open);
        BK_ENUM_MAPPING(toggle_show_inventory);
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
        BK_ENUM_MAPPING(run_n);
        BK_ENUM_MAPPING(run_ne);
        BK_ENUM_MAPPING(run_e);
        BK_ENUM_MAPPING(run_se);
        BK_ENUM_MAPPING(run_s);
        BK_ENUM_MAPPING(run_sw);
        BK_ENUM_MAPPING(run_w);
        BK_ENUM_MAPPING(run_nw);
        BK_ENUM_MAPPING(move_up);
        BK_ENUM_MAPPING(move_down);
        BK_ENUM_MAPPING(get_all_items);
        BK_ENUM_MAPPING(get_items);
        BK_ENUM_MAPPING(reset_zoom);
        BK_ENUM_MAPPING(reset_view);
        BK_ENUM_MAPPING(cancel);
        BK_ENUM_MAPPING(confirm);
        BK_ENUM_MAPPING(toggle);
        BK_ENUM_MAPPING(drop_one);
        BK_ENUM_MAPPING(drop_some);
        BK_ENUM_MAPPING(open);
        BK_ENUM_MAPPING(view);
        BK_ENUM_MAPPING(alt_get_items);
        BK_ENUM_MAPPING(alt_drop_some);
        BK_ENUM_MAPPING(alt_open);
        BK_ENUM_MAPPING(toggle_show_inventory);
        BK_ENUM_MAPPING(debug_toggle_regions);
        BK_ENUM_MAPPING(debug_teleport_self);
        default:
            break;
    }
    #undef BK_ENUM_MAPPING

    return "invalid command_type";
}

} //namespace boken
