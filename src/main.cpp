#include "catch.hpp"
#include <bkassert/assert.hpp>
#include "types.hpp"
#include "system.hpp"
#include "map.hpp"
#include "random.hpp"
#include "bsp_generator.hpp"
#include "hash.hpp"

#include <fstream>

namespace boken {};
namespace bk = boken;

template <typename T, typename U>
inline constexpr ptrdiff_t check_offsetof() noexcept {
    static_assert(std::is_standard_layout<T>::value
                , "Must be standard layout");
    static_assert(!std::is_function<U>::value
                , "Must not be a function");
    static_assert(!std::is_member_function_pointer<std::add_pointer_t<U>>::value
                , "Must not be a member function");
    return 0;
}

#define BK_OFFSETOF(s, m) (check_offsetof<s, decltype(s::m)>() + offsetof(s, m))

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

template <typename T>
inline constexpr std::add_const_t<T>& as_const(T& t) noexcept {
    return t;
}

template <typename T>
inline constexpr std::add_const_t<T>* as_const(T* const t) noexcept {
    return t;
}

template <typename T>
inline constexpr void call_destructor(T& t) noexcept(std::is_nothrow_destructible<T>::value) {
    t.~T();
}

template <size_t StackSize>
class basic_buffer {
public:
    explicit basic_buffer(size_t const size)
      : data_  {}
      , size_  {size <= StackSize ? StackSize : size}
      , first_ {init_storage_(data_, size_)}
    {
    }

    template <typename = std::enable_if_t<(StackSize > 0)>>
    basic_buffer() noexcept
      : basic_buffer {StackSize}
    {
    }

    ~basic_buffer() {
        size_ <= StackSize ? call_destructor(data_.s_)
                           : call_destructor(data_.d_);
    }

    auto size()  const noexcept { return size_; }
    auto data()  const noexcept { return as_const(first_); }
    auto begin() const noexcept { return data(); }
    auto end()   const noexcept { return begin() + size(); }
    auto data()        noexcept { return first_; }
    auto begin()       noexcept { return data(); }
    auto end()         noexcept { return begin() + size(); }

    char operator[](size_t const i) const noexcept {
        return *(begin() + i);
    }

    char& operator[](size_t const i) noexcept {
        return *(begin() + i);
    }
private:
    using static_t  = std::array<char, StackSize + 1>;
    using dynamic_t = std::unique_ptr<char[]>;

    union storage_t {
        storage_t() noexcept {}
        ~storage_t() {}

        static_t  s_;
        dynamic_t d_;
    } data_;

    static char* init_storage_(storage_t& data, size_t const size) {
        if (size <= StackSize) {
            new (&data.s_) static_t;
            return data.s_.data();
        } else {
            new (&data.d_) dynamic_t {new char[size]};
            return data.d_.get();
        }
    }

    size_t size_;
    char*  first_;
};

using dynamic_buffer = basic_buffer<0>;

template <size_t Size>
using static_buffer = basic_buffer<Size>;

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

namespace boken {

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
        }
    }
private:
    command_handler_t handler_;
};

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
        os.on_key([&](auto a) { on_key(a); });
        os.on_mouse_move([&](auto a) { on_mouse_move(a); });
        os.on_mouse_wheel([&](auto a, auto b) { on_mouse_wheel(a, b); });

        cmd_translator.on_command([&](auto a, auto b) { on_command(a, b); });

        generate();
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Initialization / Generation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void generate() {
        auto bsp_gen_ptr = [&] {
            bsp_generator::param_t p;
            p.width  = bk::size_type_x<int> {render_data.w};
            p.height = bk::size_type_y<int> {render_data.h};

            return make_bsp_generator(p);
        }();

        auto& bsp_gen   = *bsp_gen_ptr;
        auto& tile_data = render_data.tile_data;

        tile_data.clear();
        tile_data.resize(render_data.w * render_data.h, render_t::data_t {});
        bsp_gen.generate(rng_substantive);

        for (auto const& room : bsp_gen) {
            auto const r = room.rect;

            bool const gen_room =
                random_uniform_int(rng_substantive, 0, 10) < 3;

            uint32_t const color = 0xFF000000
                | (static_cast<uint8_t>(random_uniform_int(rng_substantive, 0, 255)) <<  0)
                | (static_cast<uint8_t>(random_uniform_int(rng_substantive, 0, 255)) <<  8)
                | (static_cast<uint8_t>(random_uniform_int(rng_substantive, 0, 255)) << 16);

            for (auto y = r.y0; y < r.y1; ++y) {
                for (auto x = r.x0; x < r.x1; ++x) {
                    auto& t = tile_data[x + y * render_data.w];

                    t.position.first  = x * render_data.tile_w;
                    t.position.second = y * render_data.tile_h;

                    t.color = color;

                    t.tex_coord.first  = 11 * render_data.tile_w;
                    t.tex_coord.second = 13 * render_data.tile_h;
                }
            }
        }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void on_key(kb_event const event) {
        if (event.went_down) {
            cmd_translator.translate(event);
        }
    }

    void on_mouse_move(mouse_event const event) {
        if (event.button_state[2]) {
            view_x_off += event.dx;
            view_y_off += event.dy;
        }
    }

    void on_mouse_wheel(int const wy, int) {
        view_scale *= (wy > 0 ? 1.1f : 0.9f);
    }

    void on_command(command_type const type, uintptr_t const data) {
        using ct = command_type;
        switch (type) {
        case ct::none:
            break;
        case ct::move_here:
            break;
        case ct::move_n:
            player.position += vec2i {0, -1};
            break;
        case ct::move_ne:
            break;
        case ct::move_e:
            player.position += vec2i {1, 0};
            break;
        case ct::move_se:
            break;
        case ct::move_s:
            player.position += vec2i {0, 1};
            break;
        case ct::move_sw:
            break;
        case ct::move_w:
            player.position += vec2i {-1, 0};
            break;
        case ct::move_nw:
            break;
        case ct::reset_zoom:
            break;
        case ct::reset_view:
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

        os.render_set_transform(view_scale, view_scale
                              , view_x_off, view_y_off);

        os.render_set_tile_size(render_data.tile_w, render_data.tile_h);

        // Map tiles
        os.render_set_data(render_data::position, read_only_pointer_t {
            render_data.tile_data, BK_OFFSETOF(render_t::data_t, position)});
        os.render_set_data(render_data::texture, read_only_pointer_t {
            render_data.tile_data, BK_OFFSETOF(render_t::data_t, tex_coord)});
        os.render_set_data(render_data::color, read_only_pointer_t {
            render_data.tile_data, BK_OFFSETOF(render_t::data_t, color)});
        os.render_data_n(render_data.w * render_data.h);

        // Player and entities.
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

    float view_x_off = 0.0f;
    float view_y_off = 0.0f;
    float view_scale = 1.0f;

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
};

} // namespace boken

int main(int const argc, char const* argv[]) try {
    bk::run_unit_tests();

    bk::game_state game;
    game.run();

    return 0;
} catch (...) {
    return 1;
}
