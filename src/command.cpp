#include "command.hpp"
#include "system.hpp" // kb_event

namespace boken {

command_translator::~command_translator() = default;

class command_translator_impl final : public command_translator {
public:
    command_translator_impl();
    void on_command(command_handler_t handler) final override;
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

void command_translator_impl::translate(kb_event const& event) const {
    switch (event.scancode) {
    case 79 : // SDL_SCANCODE_RIGHT = 79
        handler_(command_type::move_e, 0);
        break;
    case 80 : // SDL_SCANCODE_LEFT = 80
        handler_(command_type::move_w, 0);
        break;
    case 81 : // SDL_SCANCODE_DOWN = 81
        handler_(command_type::move_s, 0);
        break;
    case 82 : // SDL_SCANCODE_UP = 82
        handler_(command_type::move_n, 0);
        break;
    case 89 : // SDL_SCANCODE_KP_1 = 89
        handler_(command_type::move_sw, 0);
        break;
    case 90 : // SDL_SCANCODE_KP_2 = 90
        handler_(command_type::move_s, 0);
        break;
    case 91 : // SDL_SCANCODE_KP_3 = 91
        handler_(command_type::move_se, 0);
        break;
    case 92 : // SDL_SCANCODE_KP_4 = 92
        handler_(command_type::move_w, 0);
        break;
    case 93 : // SDL_SCANCODE_KP_5 = 93
        handler_(command_type::move_here, 0);
        break;
    case 94 : // SDL_SCANCODE_KP_6 = 94
        handler_(command_type::move_e, 0);
        break;
    case 95 : // SDL_SCANCODE_KP_7 = 95
        handler_(command_type::move_nw, 0);
        break;
    case 96 : // SDL_SCANCODE_KP_8 = 96
        handler_(command_type::move_n, 0);
        break;
    case 97 : // SDL_SCANCODE_KP_9 = 97
        handler_(command_type::move_ne, 0);
        break;
    case 74 : // SDL_SCANCODE_HOME = 74
        handler_(command_type::reset_view, 0);
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
        BK_ENUM_MAPPING(reset_zoom);
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
        BK_ENUM_MAPPING(reset_zoom);
    }
    #undef BK_ENUM_MAPPING

    return "invalid command_type";
}

} //namespace boken
