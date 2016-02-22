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

        cmd_translator.on_command([&](command_type a, uintptr_t b) { on_command(a, b); });

        generate();
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Initialization / Generation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void generate() {
        auto const level_w = 100;
        auto const level_h = 80;

        auto& current_level = the_world.add_new_level(nullptr
          , make_level(rng_substantive, sizeix {level_w}, sizeiy {level_h}));

        renderer.update_map_data(current_level);

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
    }

    void show_tool_tip(point2i const p) {
        tool_tip.move_to(value_cast(p.x), value_cast(p.y));

        auto const q  = current_view.window_to_world(p);
        auto const tx = floor_as<int>(value_cast(q.x) / 18);
        auto const ty = floor_as<int>(value_cast(q.y) / 18);

        if (tool_tip.visible(true) && tx == last_tile_x && ty == last_tile_y) {
            return;
        }

        auto const& tile = the_world.current_level().at(tx, ty);
        tool_tip.layout(trender, std::to_string(tile.region_id));
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void on_key(kb_event const event, kb_modifiers const kmods) {
        if (event.went_down) {
            if (kmods.test(kb_modifiers::m_shift)) {
                show_tool_tip(point2i {last_mouse_x, last_mouse_y});
            }
            cmd_translator.translate(event);
        } else {
            if (!kmods.test(kb_modifiers::m_shift)) {
                tool_tip.visible(false);
            }
        }
    }

    void on_mouse_move(mouse_event const event, kb_modifiers const kmods) {
        if (kmods.none() && event.button_state[2]) {
            current_view.x_off += event.dx;
            current_view.y_off += event.dy;
        }

        if (kmods.test(kb_modifiers::m_shift)) {
            show_tool_tip(point2i {event.x, event.y});
        }

        last_mouse_x = event.x;
        last_mouse_y = event.y;

        auto const p = current_view.window_to_world(event.x, event.y);

        last_tile_x = floor_as<int>(value_cast(p.x) / 18);
        last_tile_y = floor_as<int>(value_cast(p.y) / 18);
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

        auto const delta = clock_t::now() - last_frame;
        auto const now = clock_t::now();
        if (now - last_frame < 1s / 60) {
            return;
        }

        auto const tw = value_cast(render_data.base_tile_map.tile_w);
        auto const th = value_cast(render_data.base_tile_map.tile_h);

        os.render_clear();
        renderer.render(delta, current_view);

        //
        // Player and entities.
        //
        auto const& current_level = the_world.current_level();
        auto const& epos = current_level.entity_positions();
        auto const& eids = current_level.entity_ids();

        render_data.entity_data.clear();
        for (size_t i = 0; i < epos.size(); ++i) {
            render_data.entity_data.push_back(render_t::data_t {
                {value_cast(epos[i].x) * tw
               , value_cast(epos[i].y) * th}
              , {1 * tw, 0}
              , 0xFF
            });
        }

        os.render_set_data(render_data_type::position, read_only_pointer_t {
            render_data.entity_data, BK_OFFSETOF(render_t::data_t, position)});
        os.render_set_data(render_data_type::texture, read_only_pointer_t {
            render_data.entity_data, BK_OFFSETOF(render_t::data_t, tex_coord)});
        os.render_set_data(render_data_type::color, read_only_pointer_t {
            render_data.entity_data, BK_OFFSETOF(render_t::data_t, color)});
        os.render_data_n(render_data.entity_data.size());

        //
        // text
        //

        os.render_set_transform(1.0f, 1.0f, 0.0f, 0.0f);
        tool_tip.render(os, trender);

        auto i = 0;
        std::for_each(
            message_window.visible_begin(), message_window.visible_end()
          , [&](auto const& line) {
                os.render_set_transform(1.0f, 1.0f, 0.0f, i++ * 18.0f);
                line.render(os, trender);
            });

        os.render_present();

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
        up<game_renderer> renderer_ptr        = make_game_renderer(*system_ptr);
        up<text_renderer> trender_ptr         = make_text_renderer();
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

    struct render_t {
        tile_map base_tile_map;

        struct data_t {
            std::pair<uint16_t, uint16_t> position;
            std::pair<uint16_t, uint16_t> tex_coord;
            uint32_t                      color;
        };

        std::vector<data_t> tile_data;
        std::vector<data_t> entity_data;
    } render_data;

    message_log message_window {trender};

    text_layout tool_tip;
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
