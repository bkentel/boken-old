#include "catch.hpp"        // for run_unit_tests
#include "entity.hpp"       // for entity
#include "entity_def.hpp"   // for entity_definition
#include "hash.hpp"         // for djb2_hash_32
#include "item_def.hpp"     // for item_definition
#include "level.hpp"        // for level, placement_result, make_level, etc
#include "math.hpp"         // for vec2i, floor_as, point2f, basic_2_tuple, etc
#include "message_log.hpp"  // for message_log
#include "random.hpp"       // for random_state, make_random_state
#include "render.hpp"       // for game_renderer
#include "system.hpp"       // for system, kb_modifiers, mouse_event, etc
#include "text.hpp"         // for text_layout, text_renderer
#include "tile.hpp"         // for tile_map
#include "types.hpp"        // for value_cast, tag_size_type_x, etc
#include "utility.hpp"      // for BK_OFFSETOF
#include "world.hpp"        // for world, make_world

#include <algorithm>        // for move
#include <chrono>           // for microseconds, operator-, duration, etc
#include <functional>       // for function
#include <memory>           // for unique_ptr, allocator
#include <ratio>            // for ratio
#include <string>           // for string, to_string
#include <utility>          // for pair, make_pair
#include <vector>           // for vector

#include <cstdint>          // for uint16_t, uint32_t, uintptr_t
#include <cstdio>           // for printf

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

class item {
public:
    item(item_instance_id const i_instance, item_id const i_id)
      : instance_id {i_instance}
      , id          {i_id}
    {
    }

    item_instance_id instance_id {0};
    item_id          id          {0};
};

placement_result create_item_at(point2i const p, world& w, level& l, item_definition const& def) {
    auto const id     = w.create_item_id();
    auto const result = l.add_item_at(item {id, def.id}, p);

    switch (result) {
    case placement_result::ok :
        break;
    case placement_result::failed_bounds   :
    case placement_result::failed_entity   :
    case placement_result::failed_obstacle :
        w.free_item_id(id);
        break;
    }

    return result;
}

placement_result create_entity_at(point2i const p, world& w, level& l, entity_definition const& def) {
    auto const id     = w.create_entity_id();
    auto const result = l.add_entity_at(entity {id, def.id}, p);

    switch (result) {
    case placement_result::ok :
        break;
    case placement_result::failed_bounds   :
    case placement_result::failed_entity   :
    case placement_result::failed_obstacle :
        w.free_entity_id(id);
        break;
    }

    return result;
}

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
        os.on_key([&](kb_event a, kb_modifiers b) { on_key(a, b); });
        os.on_mouse_move([&](mouse_event a, kb_modifiers b) { on_mouse_move(a, b); });
        os.on_mouse_wheel([&](int a, int b, kb_modifiers c) { on_mouse_wheel(a, b, c); });
        os.on_mouse_button([&](mouse_event a, kb_modifiers b) { on_mouse_button(a, b); });

        cmd_translator.on_command([&](command_type a, uintptr_t b) { on_command(a, b); });

        renderer.set_message_window(&message_window);

        generate();
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Utility / Helpers
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    point2i window_to_world(point2i const p) const noexcept {
        auto const& tile_map = renderer.base_tile_map();
        auto const tw = value_cast(tile_map.tile_w);
        auto const th = value_cast(tile_map.tile_h);

        auto const q  = current_view.window_to_world(p);
        auto const tx = floor_as<int32_t>(value_cast(q.x) / tw);
        auto const ty = floor_as<int32_t>(value_cast(q.y) / th);

        return {tx, ty};
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Initialization / Generation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void generate() {
        auto const level_w = 100;
        auto const level_h = 80;

        auto& current_level = the_world.add_new_level(nullptr
          , make_level(rng_substantive, sizeix {level_w}, sizeiy {level_h}));

        // player
        create_entity_at(point2i {0, 0}, the_world, current_level, entity_definition {});

        for (size_t i = 0; i < current_level.region_count(); ++i) {
            auto const& region = current_level.region(i);
            if (region.tile_count <= 0) {
                continue;
            }

            point2i const p {region.bounds.x0 + region.bounds.width()  / 2
                           , region.bounds.y0 + region.bounds.height() / 2};

            create_entity_at(p, the_world, current_level, entity_definition {entity_id {1}});
        }

        renderer.update_map_data(current_level);
        renderer.update_entity_data(current_level);
    }

    void show_tool_tip(point2i const p) {
        renderer.update_tool_tip_visible(true);
        renderer.update_tool_tip_position(p);

        auto const q = window_to_world(p);
        if (q == point2i {last_tile_x, last_tile_y}) {
            return;
        }

        auto const& tile      = the_world.current_level().at(q);
        auto const& region_id = tile.region_id;

        renderer.update_tool_tip_text(std::to_string(region_id));
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void on_key(kb_event const event, kb_modifiers const kmods) {
        if (event.went_down) {
            if (kmods.test(kb_modifiers::m_shift)) {
                show_tool_tip({last_mouse_x, last_mouse_y});
            }
            cmd_translator.translate(event);
        }
    }

    void on_mouse_button(mouse_event const event, kb_modifiers const kmods) {
        if (event.button_state[0]) {
            auto&      lvl = the_world.current_level();
            auto const p   = window_to_world({event.x, event.y});

            if (!intersects(lvl.bounds(), p)) {
                return;
            }

            tile_data_set const data {
                tile_data {}
              , tile_flags {0}
              , tile_id::tunnel
              , tile_type::tunnel
              , 0
            };

            renderer.update_map_data(
                lvl.update_tile_at(rng_superficial, p, data));
        }
    }

    void on_mouse_move(mouse_event const event, kb_modifiers const kmods) {
        if (kmods.none() && event.button_state[2]) {
            current_view.x_off += event.dx;
            current_view.y_off += event.dy;
        }

        if (kmods.test(kb_modifiers::m_shift)) {
            show_tool_tip({event.x, event.y});
        }

        last_mouse_x = event.x;
        last_mouse_y = event.y;

        auto const p = window_to_world({event.x, event.y});
        last_tile_x = value_cast(p.x);
        last_tile_y = value_cast(p.y);
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
        case ct::move_n  : move_player(vec2i { 0, -1}); break;
        case ct::move_ne : move_player(vec2i { 1, -1}); break;
        case ct::move_e  : move_player(vec2i { 1,  0}); break;
        case ct::move_se : move_player(vec2i { 1,  1}); break;
        case ct::move_s  : move_player(vec2i { 0,  1}); break;
        case ct::move_sw : move_player(vec2i {-1,  1}); break;
        case ct::move_w  : move_player(vec2i {-1,  0}); break;
        case ct::move_nw : move_player(vec2i {-1, -1}); break;
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
    void move_player(vec2i const v) {
        message_window.println(std::to_string(value_cast(v.x)) + " " + std::to_string(value_cast(v.y)));

        the_world.current_level().move_by(entity_instance_id {}, v);
        advance(1);
    }

    void advance(int const steps) {
        renderer.update_entity_data(the_world.current_level());
    }

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

        auto const now   = clock_t::now();
        auto const delta = now - last_frame;
        if (delta < 1s / 60) {
            return;
        }

        renderer.render(delta, current_view);

        last_frame_time = now;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Data
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    struct state_t {
        template <typename T>
        using up = std::unique_ptr<T>;

        up<system>        system_ptr          = make_system();
        up<random_state>  rng_substantive_ptr = make_random_state();
        up<random_state>  rng_superficial_ptr = make_random_state();
        up<world>         world_ptr           = make_world();
        up<text_renderer> trender_ptr         = make_text_renderer();
        up<game_renderer> renderer_ptr        = make_game_renderer(*system_ptr, *trender_ptr);
    } state {};

    system&        os              = *state.system_ptr;
    random_state&  rng_substantive = *state.rng_substantive_ptr;
    random_state&  rng_superficial = *state.rng_superficial_ptr;
    world&         the_world       = *state.world_ptr;
    game_renderer& renderer        = *state.renderer_ptr;
    text_renderer& trender         = *state.trender_ptr;

    command_translator cmd_translator {};

    view current_view;

    int last_mouse_x = 0;
    int last_mouse_y = 0;
    int last_tile_x  = 0;
    int last_tile_y  = 0;

    timepoint_t last_frame_time {};

    message_log message_window {trender};
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
