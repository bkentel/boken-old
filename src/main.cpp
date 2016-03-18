#include "catch.hpp"        // for run_unit_tests
#include "command.hpp"
#include "data.hpp"
#include "entity.hpp"       // for entity
#include "entity_def.hpp"   // for entity_definition
#include "hash.hpp"         // for djb2_hash_32
#include "item.hpp"
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

template <typename T>
inline T make_id(string_view const s) noexcept {
    return T {djb2_hash_32(s.begin(), s.end())};
}

template <typename T, size_t N>
inline constexpr T make_id(char const (&s)[N]) noexcept {
    return T {djb2_hash_32c(s)};
}

struct game_state {
    enum class placement_type {
        at, near
    };

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
        os.on_text_input([&](text_input_event const e) { on_text_input(e); });

        cmd_translator.on_command([&](command_type a, uintptr_t b) { on_command(a, b); });

        renderer.set_message_window(&message_window);

        renderer.set_tile_maps({
            {tile_map_type::base,   database.get_tile_map(tile_map_type::base)}
          , {tile_map_type::entity, database.get_tile_map(tile_map_type::entity)}
          , {tile_map_type::item,   database.get_tile_map(tile_map_type::item)}
        });

        generate();
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Utility / Helpers
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    point2i window_to_world(point2i const p) const noexcept {
        auto const& tile_map = database.get_tile_map(tile_map_type::base);
        auto const tw = value_cast<float>(tile_map.tile_width());
        auto const th = value_cast<float>(tile_map.tile_height());

        auto const q  = current_view.window_to_world(p);
        auto const tx = floor_as<int32_t>(value_cast(q.x) / tw);
        auto const ty = floor_as<int32_t>(value_cast(q.y) / th);

        return {tx, ty};
    }

    // @param p Position in world coordinates
    void update_tile_at(point2i const p) {
        auto& lvl = the_world.current_level();

        if (!intersects(lvl.bounds(), p)) {
            return;
        }

        if (lvl.at(p).type == tile_type::tunnel) {
            return;
        }

        tile_data_set const data {
            tile_data {}
          , tile_flags {0}
          , tile_id::tunnel
          , tile_type::tunnel
          , 0
        };

        renderer.update_map_data(lvl.update_tile_at(rng_superficial, p, data));
    }

    //! @param p Position in window coordinates
    void show_tool_tip(point2i const p) {
        auto const p0 = window_to_world(p);
        auto const q  = window_to_world({last_mouse_x, last_mouse_y});

        auto const was_visible = renderer.update_tool_tip_visible(true);
        renderer.update_tool_tip_position(p);

        if (was_visible && p0 == q) {
            return; // the tile the mouse points to is unchanged from last time
        }

        auto  const& lvl    = the_world.current_level();
        auto  const& tile   = lvl.at(p0);

        static_string_buffer<256> buffer;

        auto const print_entity = [&]() noexcept {
            auto* const entity = lvl.entity_at(p0);
            if (!entity) {
                return true;
            }

            auto* const def = database.find(entity->definition());

            return buffer.append(
                "Entities:\n"
                " Instance  : %0#10x\n"
                " Definition: %0#10x (%s)\n"
                " Name      : %s\n"
              , value_cast(entity->instance())
              , value_cast(entity->definition()), (def ? def->id_string.c_str() : "{empty}")
              , (def ? def->name.c_str() : "{empty}"));
        };

        auto const print_items = [&]() noexcept {
            auto* const pile = lvl.item_at(p0);
            if (!pile) {
                return true;
            }

            buffer.append("Items:\n");

            int i = 0;
            for (auto const& id : *pile) {
                if (!buffer.append(" Item: %d\n", i++)) {
                    break;
                }

                auto* const itm = the_world.find(id);
                BK_ASSERT(!!itm);

                auto* const def = database.find(itm->definition());

                buffer.append(
                    " Instance  : %0#10x\n"
                    " Definition: %0#10x (%s)\n"
                    " Name      : %s\n"
                  , value_cast(itm->instance())
                  , value_cast(itm->definition()), (def ? def->id_string.c_str() : "{empty}")
                  , (def ? def->name.c_str() : "{empty}"));
            }

            return !!buffer;
        };

        auto const result =
            buffer.append(
                "Position: %d, %d\n"
                "Region  : %d\n"
                "Tile    : %s\n"
              , value_cast(p0.x), value_cast(p0.y)
              , tile.region_id
              , enum_to_string(lvl.at(p0).id).data())
         && print_entity()
         && print_items();

        renderer.update_tool_tip_text(buffer.to_string());
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Initialization / Generation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void generate_player() {
        auto& lvl = the_world.current_level();

        auto const def = database.find(make_id<entity_id>("player"));
        BK_ASSERT(!!def);

        auto const result = add_entity_at(create_entity(*def), lvl.stair_up(0));
        BK_ASSERT(result.second == placement_result::ok);
    }

    void generate_entities() {
        auto& lvl = the_world.current_level();

        auto const def = database.find(make_id<entity_id>("rat_small"));
        BK_ASSERT(!!def);

        auto ent = create_entity(*def);

        for (size_t i = 0; i < lvl.region_count(); ++i) {
            auto const& region = lvl.region(i);
            if (region.tile_count <= 0) {
                continue;
            }

            point2i const p {region.bounds.x0 + region.bounds.width()  / 2
                           , region.bounds.y0 + region.bounds.height() / 2};

            auto const result = add_entity_near(std::move(ent), p, 3, rng_substantive);
            if (result.second == placement_result::ok) {
                ent.reset(create_entity(*def).release());
            }
        }
    }

    void generate_items() {
        auto& lvl = the_world.current_level();

        auto const def = database.find(make_id<item_id>("dagger"));
        BK_ASSERT(!!def);

        auto itm = create_item(*def);

        for (size_t i = 0; i < lvl.region_count(); ++i) {
            auto const& region = lvl.region(i);
            if (region.tile_count <= 0) {
                continue;
            }

            point2i const p {region.bounds.x0 + region.bounds.width()  / 2
                           , region.bounds.y0 + region.bounds.height() / 2};

            auto const result = add_item_near(std::move(itm), p, 3, rng_substantive);
            if (result.second == placement_result::ok) {
                itm.reset(create_item(*def).release());
            }
        }
    }

    void generate_level(level* const parent, size_t const id) {
        auto const level_w = 100;
        auto const level_h = 80;

        the_world.add_new_level(parent
          , make_level(rng_substantive, the_world, sizeix {level_w}, sizeiy {level_h}, id));

        the_world.change_level(id);
    }

    void generate(size_t const id = 0) {
        if (id == 0) {
            generate_level(nullptr, id);
            generate_player();
        } else {
            generate_level(&the_world.current_level(), id);
        }

        generate_entities();
        generate_items();

        set_current_level(id);
    }

    void set_current_level(size_t const id) {
        BK_ASSERT(the_world.has_level(id));
        renderer.set_level(the_world.change_level(id));

        item_updates_.clear();
        entity_updates_.clear();

        renderer.update_map_data();
        renderer.update_entity_data();
        renderer.update_item_data();
    }

    void reset_view_to_player() {
        auto const& tile_map = database.get_tile_map(tile_map_type::base);
        auto const tw = value_cast<float>(tile_map.tile_width());
        auto const th = value_cast<float>(tile_map.tile_height());

        current_view.scale_x = 1.0f;
        current_view.scale_y = 1.0f;

        auto const p = get_player().second;

        auto const px = value_cast(p.x) * tw + tw / 2;
        auto const py = value_cast(p.y) * th + th / 2;

        auto const r = os.render_get_client_rect();

        current_view.x_off = -px + (r.width()  / 2.0f);
        current_view.y_off = -py + (r.height() / 2.0f);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void on_key(kb_event const event, kb_modifiers const kmods) {
        if (event.went_down) {
            cmd_translator.translate(event);
        } else {
            if (!kmods.test(kb_modifiers::m_shift)) {
                renderer.update_tool_tip_visible(false);
            }
        }
    }

    void on_text_input(text_input_event const event) {
        cmd_translator.translate(event);
    }

    void on_mouse_button(mouse_event const event, kb_modifiers const kmods) {
        switch (event.button_state_bits()) {
        case 0b0000 :
            // no buttons down
            break;
        case 0b0001 :
            // left mouse button only
            if (event.button_change[0] == mouse_event::button_change_t::went_down) {
                update_tile_at(window_to_world({event.x, event.y}));
            }
            break;
        case 0b0010 :
            // middle mouse button only
            break;
        case 0b0100 :
            // right mouse button only
            break;
        case 0b1000 :
            // extra mouse button only
            break;
        default :
            break;
        }
    }

    void on_mouse_move(mouse_event const event, kb_modifiers const kmods) {
        switch (event.button_state_bits()) {
        case 0b0000 :
            // no buttons down
            if (kmods.test(kb_modifiers::m_shift)) {
                show_tool_tip({event.x, event.y});
            }
            break;
        case 0b0001 :
            // left mouse button only
            break;
        case 0b0010 :
            // middle mouse button only
            break;
        case 0b0100 :
            // right mouse button only
            if (kmods.none()) {
                current_view.x_off += static_cast<float>(event.dx);
                current_view.y_off += static_cast<float>(event.dy);
            }
            break;
        case 0b1000 :
            // extra mouse button only
            break;
        default :
            break;
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
        case ct::move_n  : player_move(vec2i { 0, -1}); break;
        case ct::move_ne : player_move(vec2i { 1, -1}); break;
        case ct::move_e  : player_move(vec2i { 1,  0}); break;
        case ct::move_se : player_move(vec2i { 1,  1}); break;
        case ct::move_s  : player_move(vec2i { 0,  1}); break;
        case ct::move_sw : player_move(vec2i {-1,  1}); break;
        case ct::move_w  : player_move(vec2i {-1,  0}); break;
        case ct::move_nw : player_move(vec2i {-1, -1}); break;
        case ct::get_all_items : get_all_items(); break;
        case ct::move_down : do_change_level(); break;
        case ct::move_up   : do_change_level(); break;
        case ct::reset_view : reset_view_to_player(); break;
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
    std::pair<entity const&, point2i> get_player() const {
        return const_cast<game_state*>(this)->get_player();
    }

    std::pair<entity&, point2i> get_player() {
        auto const result = the_world.current_level().find(entity_instance_id {1});
        BK_ASSERT(result.first);
        return {*result.first, result.second};
    }

    void get_all_items() {
        auto& lvl = the_world.current_level();
        auto const player = get_player();

        static_string_buffer<128> buffer;

        auto const result = lvl.move_items(player.second, player.first
          , [&](item_instance_id const id) noexcept {
                auto* const itm = the_world.find(id);
                BK_ASSERT(!!itm);

                auto* const idef = database.find(itm->definition());

                buffer.clear();
                buffer.append("Picked up %s.", idef ? idef->name.c_str() : "{unknown}");

                message_window.println(buffer.to_string());

                return item_merge_result::ok;
            });

        switch (result) {
        case merge_item_result::ok_merged_none:
            break;
        case merge_item_result::ok_merged_some:
        case merge_item_result::ok_merged_all:
            item_updates_.push_back({player.second, player.second, item_id {}});
            break;
        case merge_item_result::failed_bad_source:
            message_window.println("There is nothing here to get.");
            break;
        case merge_item_result::failed_bad_destination:
        default:
            BK_ASSERT(false);
            break;
        }
    }

    unique_entity create_entity(entity_definition const& def) {
        return the_world.create_entity([&](entity_instance_id const instance) {
            return entity {instance, def.id};
        });
    }

    unique_item create_item(item_definition const& def) {
        return the_world.create_item([&](item_instance_id const instance) {
            return item {instance, def.id};
        });
    }

    using placement_pair = std::pair<point2i, placement_result>;

    placement_pair add_item_near(unique_item&& i, point2i const p, int32_t const distance, random_state& rng) {
        auto const itm = the_world.find(i.get());
        BK_ASSERT(!!itm);

        item_id const def = itm->definition();

        auto const result = the_world.current_level()
          .add_item_nearest_random(rng, std::move(i), p, distance);

        if (result.second == placement_result::ok) {
            item_updates_.push_back({p, p, def});
        }

        return result;
    }

    placement_pair add_entity_near(unique_entity&& e, point2i const p, int32_t const distance, random_state& rng) {
        auto const ent = the_world.find(e.get());
        BK_ASSERT(!!ent);

        entity_id const def = ent->definition();

        auto const result = the_world.current_level()
          .add_entity_nearest_random(rng, std::move(e), p, distance);

        if (result.second == placement_result::ok) {
            entity_updates_.push_back({p, p, def});
        }

        return result;
    }

    placement_pair add_item_at(unique_item&& i, point2i const p) {
        return add_item_near(std::move(i), p, 0, rng_superficial);
    }

    placement_pair add_entity_at(unique_entity&& e, point2i const p) {
        return {p, the_world.current_level().add_entity_at(std::move(e), p)};
    }

    void do_combat(point2i const att_pos, point2i const def_pos) {
        auto& lvl = the_world.current_level();

        auto* const att = lvl.entity_at(att_pos);
        auto* const def = lvl.entity_at(def_pos);

        BK_ASSERT(!!att && !!def);
        BK_ASSERT(att->is_alive() && def->is_alive());

        def->modify_health(-1);

        if (!def->is_alive()) {
            auto const idef = database.find(make_id<item_id>("dagger"));
            BK_ASSERT(!!idef);

            add_item_at(create_item(*idef), def_pos);
            lvl.remove_entity(def->instance());
            entity_updates_.push_back({def_pos, def_pos, entity_id {}});
        }

        advance(1);
    }

    void do_change_level() {
        auto const player = get_player();
        auto const tile   = the_world.current_level().at(player.second);

        auto const delta = tile.id == tile_id::stair_down ?  1
                         : tile.id == tile_id::stair_up   ? -1
                                                          :  0;

        if (delta == 0) {
            message_window.println("There are no stairs here.");
            return;
        }

        auto const id = the_world.current_level().id();
        if (id == 0 && delta < 0) {
            message_window.println("You can't leave.");
            return;
        }

        auto player_ent = the_world.current_level()
          .remove_entity(player.first.instance());

        if (!the_world.has_level(id + delta)) {
            generate(id + delta);
        } else {
            set_current_level(id + delta);
        }

        auto const p = (delta > 0)
          ? the_world.current_level().stair_up(0)
          : the_world.current_level().stair_down(0);

        add_entity_near(std::move(player_ent), p, 5, rng_substantive);
    }

    void interact_obstacle(entity& e, point2i const cur_pos, point2i const obstacle_pos) {
        auto& lvl = the_world.current_level();

        auto const tile = lvl.at(obstacle_pos);
        if (tile.type == tile_type::door) {
            auto const id = (tile.id == tile_id::door_ns_closed)
                ? tile_id::door_ns_open
                : tile_id::door_ew_open;

            tile_data_set const data {
                tile_data {}
                , tile_flags {0}
                , id
                , tile.type
                , 0
            };

            renderer.update_map_data(lvl.update_tile_at(rng_superficial, obstacle_pos, data));
        }
    }

    void player_move(vec2i const v) {
        auto& lvl = the_world.current_level();

        auto const player = get_player();
        auto const p0 = player.second;
        auto const p1 = p0 + v;

        switch (lvl.move_by(player.first.instance(), v)) {
        case placement_result::ok:
            entity_updates_.push_back({p0, p1, player.first.definition()});
            advance(1);
            break;
        case placement_result::failed_entity:
            do_combat(p0, p1);
            break;
        case placement_result::failed_obstacle:
            interact_obstacle(player.first, p0, p1);
            break;
        case placement_result::failed_bounds: break;
        case placement_result::failed_bad_id: break;
        default: break;
        }
    }

    void advance(int const steps) {
        auto& lvl = the_world.current_level();

        lvl.transform_entities(
            [&](entity& e, point2i const p) noexcept {
                // the player
                if (e.instance() == entity_instance_id {1}) {
                    return p;
                }

                auto const temperment =
                    e.property_value_or(database, entity_property::temperment, 0);

                if (!random_chance_in_x(rng_superficial, 1, 10)) {
                    return p;
                }

                constexpr std::array<int, 4> dir_x {-1,  0, 0, 1};
                constexpr std::array<int, 4> dir_y { 0, -1, 1, 0};

                auto const dir = static_cast<size_t>(random_uniform_int(rng_superficial, 0, 3));
                auto const d   = vec2i {dir_x[dir], dir_y[dir]};

                return p + d;
            }
          , [&](entity& e, point2i const p, point2i const q) noexcept {
                entity_updates_.push_back({p, q, e.definition()});
            }
        );
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

        if (!entity_updates_.empty()) {
            renderer.update_entity_data(entity_updates_);
        }

        if (!item_updates_.empty()) {
            renderer.update_item_data(item_updates_);
        }

        renderer.render(delta, current_view);

        last_frame_time = now;

        entity_updates_.clear();
        item_updates_.clear();
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Data
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    struct state_t {
        template <typename T>
        using up = std::unique_ptr<T>;

        up<system>             system_ptr          = make_system();
        up<random_state>       rng_substantive_ptr = make_random_state();
        up<random_state>       rng_superficial_ptr = make_random_state();
        up<game_database>      database_ptr        = make_game_database();
        up<world>              world_ptr           = make_world();
        up<text_renderer>      trender_ptr         = make_text_renderer();
        up<game_renderer>      renderer_ptr        = make_game_renderer(*system_ptr, *trender_ptr);
        up<command_translator> cmd_translator_ptr  = make_command_translator();
    } state {};

    system&             os              = *state.system_ptr;
    random_state&       rng_substantive = *state.rng_substantive_ptr;
    random_state&       rng_superficial = *state.rng_superficial_ptr;
    game_database&      database        = *state.database_ptr;
    world&              the_world       = *state.world_ptr;
    game_renderer&      renderer        = *state.renderer_ptr;
    text_renderer&      trender         = *state.trender_ptr;
    command_translator& cmd_translator  = *state.cmd_translator_ptr;

    view current_view;

    int last_mouse_x = 0;
    int last_mouse_y = 0;

    std::vector<game_renderer::update_t<item_id>>   item_updates_;
    std::vector<game_renderer::update_t<entity_id>> entity_updates_;

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
