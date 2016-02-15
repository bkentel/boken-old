#include "catch.hpp"
#include <bkassert/assert.hpp>
#include "types.hpp"
#include "system.hpp"
#include "map.hpp"
#include "random.hpp"
#include "bsp_generator.hpp"
#include "hash.hpp"
#include "utility.hpp"
#include "text.hpp"
#include "level.hpp"

#include <fstream>

namespace boken {};
namespace bk = boken;

namespace boken {

struct keydef_t {
    enum class key_t {
        none, scan_code, virtual_key
    };

    struct hash_t {
        uint32_t operator()(keydef_t const& k) const noexcept {
            return bk::djb2_hash_32(k.name.data());
        }
    };

    keydef_t(std::string name_, uint32_t const value_, key_t const type_)
      : name  {std::move(name_)}
      , value {value_}
      , hash  {hash_t {}(*this)}
      , type  {type_}
    {
    }

    bool operator==(keydef_t const& other) const noexcept {
        return type == other.type && hash == other.hash;
    }

    std::string name;
    uint32_t    value;
    uint32_t    hash;
    key_t       type;
};

char const* load_keyboard_names() {
    static constexpr char end_of_data = 0xFF;

    std::ifstream in {"./data/keyboard-names.dat", std::ios::binary};
    auto const last = in.seekg(0, std::ios::end).tellg();
    auto const size = static_cast<size_t>(last - in.seekg(0).tellg());

    dynamic_buffer buffer {size + 1};
    in.read(buffer.data(), size);
    buffer[size] = end_of_data;

    std::vector<keydef_t> result;

    char const* p         = buffer.begin();
    char const* error     = nullptr;
    auto        type      = keydef_t::key_t::none;
    char const* name_beg  = nullptr;
    char const* name_end  = nullptr;
    char const* value_beg = nullptr;
    char const* value_end = nullptr;

    auto const parse_value = [&] {
        for (bool in_string = false; ; ++p) switch (*p) {
        case ' ' :
        case '\t':
        case '\r':
            break;
        case '\n':
        case end_of_data:
            return;
        default:
            if (!in_string && (in_string = true)) {
                value_beg = value_end = p;
            } else {
                ++value_end;
            }
            break;
        }
    };

    auto const parse_name = [&] {
        for (bool in_string = false; ; ++p) switch (*p) {
        case ' ' :
        case '\t':
            if (in_string) {
                parse_value();
                return;
            }
            break;
        case end_of_data :
            error = "Unexpected end of data.";
            return;
        default:
            if (!in_string && (in_string = true)) {
                name_beg = name_end = p;
            } else {
                ++name_end;
            }
            break;
        }
    };

    auto const parse_type = [&] {
        switch (std::tolower(*p++)) {
        case 's' : type = keydef_t::key_t::scan_code;   break;
        case 'v' : type = keydef_t::key_t::virtual_key; break;
        case end_of_data :
            error = "Unexpected end of data.";
            return;
        default  :
            error = "Expected 's' or 'v'.";
            return;
        }

        parse_name();
    };

    for ( ; ; ++p) switch (*p) {
    case ' ':
    case '\n':
    case '\r':
        break;
    case end_of_data :
        return nullptr;
    default:
        parse_type();
        if (error) {
            return error;
        }

        result.emplace_back(
            std::string {name_beg, name_end + 1}
          , std::stoul(std::string {value_beg, value_end + 1}, nullptr, 0)
          , type);

        break;
    }

    return nullptr;
}

enum class command_type {
    none

  , move_here
  , move_n
  , move_ne
  , move_e
  , move_se
  , move_s
  , move_sw
  , move_w
  , move_nw

  , reset_zoom
  , reset_view
};

class command_translator {
public:
    command_translator() {
        handler_ = [](auto, auto) noexcept {};
    }

    using command_handler_t = std::function<void (command_type, uintptr_t)>;
    void on_command(command_handler_t handler) {
        handler_ = std::move(handler);
    }

    void translate(bk::kb_event event) {
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
private:
    command_handler_t handler_;
};

class view {
public:
    view() = default;

    template <typename T>
    point2f world_to_window(T const x, T const y) const noexcept {
        return {scale_x * x + x_off
              , scale_y * y + y_off};
    }

    template <typename T>
    point2f world_to_window(vec2<T> const v) const noexcept {
        return {scale_x * value_cast(v.x)
              , scale_y * value_cast(v.y)};
    }

    template <typename T>
    point2f world_to_window(offset_type_x<T> const x, offset_type_y<T> const y) const noexcept {
        return window_to_world(value_cast(x), value_cast(y));
    }

    template <typename T>
    point2f world_to_window(point2<T> const p) const noexcept {
        return window_to_world(p.x, p.y);
    }

    template <typename T>
    point2f window_to_world(T const x, T const y) const noexcept {
        return {(1.0f / scale_x) * x - (x_off / scale_x)
              , (1.0f / scale_y) * y - (y_off / scale_y)};
    }

    template <typename T>
    vec2f window_to_world(vec2<T> const v) const noexcept {
        return {(1.0f / scale_x) * value_cast(v.x)
              , (1.0f / scale_y) * value_cast(v.y)};
    }

    template <typename T>
    point2f window_to_world(offset_type_x<T> const x, offset_type_y<T> const y) const noexcept {
        return world_to_window(value_cast(x), value_cast(y));
    }

    template <typename T>
    point2f window_to_world(point2<T> const p) const noexcept {
        return world_to_window(p.x, p.y);
    }


    template <typename T>
    void center_on_world(T const wx, T const wy) const noexcept {

    }

    float x_off   = 0.0f;
    float y_off   = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
};

class entity {
public:
    entity_id          id          {0};
    entity_instance_id instance_id {0};
};


class item {
public:
    item_id          id          {0};
    item_instance_id instance_id {0};
};

//====---
// (Singular) World state.
//====---
class world {
public:
    virtual item   const* find(item_instance_id   id) const noexcept = 0;
    virtual entity const* find(entity_instance_id id) const noexcept = 0;

    virtual item   create_item();
    virtual entity create_entity();
};

std::unique_ptr<world> make_world();

struct game_state {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Types
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    using clock_t     = std::chrono::high_resolution_clock;
    using timepoint_t = clock_t::time_point;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Special member functions
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    game_state() {
        os.on_key([&](auto a, auto b) { on_key(a, b); });
        os.on_mouse_move([&](auto a, auto b) { on_mouse_move(a, b); });
        os.on_mouse_wheel([&](auto a, auto b, auto c) { on_mouse_wheel(a, b, c); });

        cmd_translator.on_command([&](auto a, auto b) { on_command(a, b); });

        test_layout.layout(trender, "This is a test.");
        test_layout.visible(true);

        generate();
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Initialization / Generation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void generate() {
        auto const random_color_comp = [&] {
            return static_cast<uint8_t>(random_uniform_int(rng_superficial, 0, 255));
        };

        auto const random_color = [&] {
            return 0xFFu                << 24
                  | random_color_comp() << 16
                  | random_color_comp() << 8
                  | random_color_comp() << 0;
        };

        current_level = make_level(rng_substantive, sizeix {100}, sizeiy {80});
        render_data.tile_data.resize(100 * 80);

        auto const  tile_pair     = current_level->tile_indicies(0);
        auto const& tile_indicies = tile_pair.first;
        auto const  tile_rect     = tile_pair.second;
        auto const  w             = tile_rect.width();

        for (auto y = tile_rect.y0; y < tile_rect.y1; ++y) {
            for (auto x = tile_rect.x0; x < tile_rect.x1; ++x) {
                auto const& src = tile_indicies[x + y * w];
                auto&       dst = render_data.tile_data[x + y * w];

                dst.position.first  = x * render_data.tile_w;
                dst.position.second = y * render_data.tile_h;

                if (src == 11u + 13u * 16u) {
                    dst.color = random_color();
                } else {
                    dst.color = 0xFF0000FFu;
                }

                dst.tex_coord.first  = (src % render_data.tiles_x) * render_data.tile_w;
                dst.tex_coord.second = (src / render_data.tiles_x) * render_data.tile_h;
            }
        }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void on_key(kb_event const event, kb_modifiers const kmods) {
        if (event.went_down) {
            cmd_translator.translate(event);
        }
    }

    void on_mouse_move(mouse_event const event, kb_modifiers const kmods) {
        if (kmods.none() && event.button_state[2]) {
            current_view.x_off += event.dx;
            current_view.y_off += event.dy;
        }

        if (kmods.test(kb_modifiers::m_shift)) {
            test_layout.move_to(event.x, event.y);
        }

        last_mouse_x = event.x;
        last_mouse_y = event.y;
    }

    void on_mouse_wheel(int const wy, int, kb_modifiers const kmods) {
        auto const mouse_p = point2f {static_cast<float>(last_mouse_x)
                                    , static_cast<float>(last_mouse_y)};

        auto const world_mouse_p = current_view.window_to_world(mouse_p);

        current_view.scale_x *= (wy > 0 ? 1.1f : 0.9f);
        current_view.scale_y = current_view.scale_x;

        auto const v = mouse_p - current_view.world_to_window(world_mouse_p);
        current_view.x_off += value_cast(v.x);
        current_view.y_off += value_cast(v.y);
    }

    void on_command(command_type const type, uintptr_t const data) {
        using ct = command_type;
        switch (type) {
        case ct::none:
            break;
        case ct::move_here:
            break;
        case ct::move_n  : player.position += vec2i { 0, -1}; break;
        case ct::move_ne : player.position += vec2i { 1, -1}; break;
        case ct::move_e  : player.position += vec2i { 1,  0}; break;
        case ct::move_se : player.position += vec2i { 1,  1}; break;
        case ct::move_s  : player.position += vec2i { 0,  1}; break;
        case ct::move_sw : player.position += vec2i {-1,  1}; break;
        case ct::move_w  : player.position += vec2i {-1,  0}; break;
        case ct::move_nw : player.position += vec2i {-1, -1}; break;
        case ct::reset_view:
            current_view.x_off = 0.0f;
            current_view.y_off = 0.0f;
        case ct::reset_zoom:
            current_view.scale_x = 1.0f;
            current_view.scale_y = 1.0f;
            break;
        default:
            break;
        }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Simulation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void run() {
        while (os.is_running()) {
            os.do_events();
            render(last_frame_time);
        }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Rendering
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void render(timepoint_t const last_frame) {
        using namespace std::chrono_literals;

        auto const now = clock_t::now();
        if (now - last_frame < 1s / 60) {
            return;
        }

        os.render_clear();

        os.render_set_transform(current_view.scale_x, current_view.scale_y
                              , current_view.x_off,   current_view.y_off);

        os.render_set_tile_size(render_data.tile_w, render_data.tile_h);

        //
        // Map tiles
        //
        os.render_set_data(render_data::position, read_only_pointer_t {
            render_data.tile_data, BK_OFFSETOF(render_t::data_t, position)});
        os.render_set_data(render_data::texture, read_only_pointer_t {
            render_data.tile_data, BK_OFFSETOF(render_t::data_t, tex_coord)});
        os.render_set_data(render_data::color, read_only_pointer_t {
            render_data.tile_data, BK_OFFSETOF(render_t::data_t, color)});
        os.render_data_n(render_data.tile_data.size());

        //
        // Player and entities.
        //
        render_data.entity_data.clear();
        render_data.entity_data.push_back(render_t::data_t {
            {value_cast(player.position.x) * render_data.tile_w, value_cast(player.position.y) * render_data.tile_h}
          , {1 * render_data.tile_w, 0}
          , 0xFF
        });

        os.render_set_data(render_data::position, read_only_pointer_t {
            render_data.entity_data, BK_OFFSETOF(render_t::data_t, position)});
        os.render_set_data(render_data::texture, read_only_pointer_t {
            render_data.entity_data, BK_OFFSETOF(render_t::data_t, tex_coord)});
        os.render_set_data(render_data::color, read_only_pointer_t {
            render_data.entity_data, BK_OFFSETOF(render_t::data_t, color)});
        os.render_data_n(render_data.entity_data.size());

        //
        // text
        //
        os.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);
        test_layout.render(os, trender);

        os.render_present();

        last_frame_time = now;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Data
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    struct state_t {
        std::unique_ptr<system>       system_ptr          = make_system();
        std::unique_ptr<random_state> rng_substantive_ptr = make_random_state();
        std::unique_ptr<random_state> rng_superficial_ptr = make_random_state();
    } state {};

    system&       os              = *state.system_ptr;
    random_state& rng_substantive = *state.rng_substantive_ptr;
    random_state& rng_superficial = *state.rng_superficial_ptr;

    command_translator cmd_translator {};

    struct player_t {
        point2i position {0, 0};
    } player;

    view current_view;

    int last_mouse_x = 0;
    int last_mouse_y = 0;

    timepoint_t last_frame_time {};

    struct render_t {
        static constexpr int w       = 100;
        static constexpr int h       = 80;
        static constexpr int tile_w  = 18;
        static constexpr int tile_h  = 18;
        static constexpr int tiles_x = 16;
        static constexpr int tiles_y = 16;

        struct data_t {
            std::pair<uint16_t, uint16_t> position;
            std::pair<uint16_t, uint16_t> tex_coord;
            uint32_t                      color;
        };

        std::vector<data_t> tile_data;
        std::vector<data_t> entity_data;
    } render_data;

    text_renderer trender;

    text_layout test_layout;

    std::unique_ptr<level> current_level;
};

} // namespace boken

int main(int const argc, char const* argv[]) try {
    {
        using namespace std::chrono;

        auto const beg = high_resolution_clock::now();
        bk::run_unit_tests();
        auto const end = high_resolution_clock::now();

        auto const us = duration_cast<microseconds>(end - beg);
        std::printf("Tests took %lld microseconds.\n", us.count());
    }

    bk::game_state game;
    game.run();

    return 0;
} catch (...) {
    return 1;
}
