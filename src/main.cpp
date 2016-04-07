#include "catch.hpp"        // for run_unit_tests
#include "command.hpp"
#include "data.hpp"
#include "entity.hpp"       // for entity
#include "entity_def.hpp"   // for entity_definition
#include "hash.hpp"         // for djb2_hash_32
#include "item.hpp"
#include "item_def.hpp"     // for item_definition
#include "level.hpp"        // for level, placement_result, make_level, etc
#include "math.hpp"         // for vec2i32, floor_as, point2f, basic_2_tuple, etc
#include "message_log.hpp"  // for message_log
#include "random.hpp"       // for random_state, make_random_state
#include "render.hpp"       // for game_renderer
#include "system.hpp"       // for system, kb_modifiers, mouse_event, etc
#include "text.hpp"         // for text_layout, text_renderer
#include "tile.hpp"         // for tile_map
#include "types.hpp"        // for value_cast, tag_size_type_x, etc
#include "utility.hpp"      // for BK_OFFSETOF
#include "world.hpp"        // for world, make_world
#include "inventory.hpp"
#include "scope_guard.hpp"

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
#include <cinttypes>

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

enum event_result {
    filter              //!< filter the event
  , filter_detach       //!< detach and filter the event
  , pass_through        //!< pass through to the next handler
  , pass_through_detach //!< detach and pass through to the next handler
};

class input_context {
public:
    event_result on_key(kb_event const event, kb_modifiers const kmods) {
        return on_key_handler
          ? on_key_handler(event, kmods)
          : event_result::pass_through;
    }

    event_result on_text_input(text_input_event const event) {
        return on_text_input_handler
          ? on_text_input_handler(event)
          : event_result::pass_through;
    }

    event_result on_mouse_button(mouse_event const event, kb_modifiers const kmods) {
        return on_mouse_button_handler
          ? on_mouse_button_handler(event, kmods)
          : event_result::pass_through;
    }

    event_result on_mouse_move(mouse_event const event, kb_modifiers const kmods) {
        return on_mouse_move_handler
          ? on_mouse_move_handler(event, kmods)
          : event_result::pass_through;
    }

    event_result on_mouse_wheel(int const wy, int const wx, kb_modifiers const kmods) {
        return on_mouse_wheel_handler
          ? on_mouse_wheel_handler(wy, wx, kmods)
          : event_result::pass_through;
    }

    event_result on_command(command_type const type, uintptr_t const data) {
        return on_command_handler
          ? on_command_handler(type, data)
          : event_result::pass_through;
    }

    std::function<event_result (kb_event, kb_modifiers)>    on_key_handler;
    std::function<event_result (text_input_event)>          on_text_input_handler;
    std::function<event_result (mouse_event, kb_modifiers)> on_mouse_button_handler;
    std::function<event_result (mouse_event, kb_modifiers)> on_mouse_move_handler;
    std::function<event_result (int, int, kb_modifiers)>    on_mouse_wheel_handler;
    std::function<event_result (command_type, uintptr_t)>   on_command_handler;
};



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

        make_inventory();
        renderer.set_inventory_window(&inventory);

        generate();

        reset_view_to_player();
    }

    void make_inventory() {
        inventory.add_column(0, ""
          , [&](item const& itm) {
                auto const& tmap = database.get_tile_map(tile_map_type::item);
                auto const index = tmap.find(itm.definition());

                BK_ASSERT(index < 0x7Fu); //TODO

                std::array<char, 7> as_string {};
                as_string[0] =
                    static_cast<char>(static_cast<uint8_t>(index & 0x7Fu));

                return std::string {as_string.data()};
            });

        inventory.add_column(1, "Name"
          , [&](item const& itm) {
                auto const def = database.find(itm.definition());
                return def ? def->name : "{unknown}";
            });

        inventory.add_column(2, "Weight"
          , [&](item const& itm) {
                auto const weight = property_value_or(
                    database
                  , itm.definition()
                  , make_id<item_property_id>("weight")
                  , item_property_value {0});

                return std::to_string(weight);
            });

        inventory.add_column(3, "Id"
          , [&](item const& itm) {
                auto const def = database.find(itm.definition());
                return def ? def->id_string : "{unknown}";
            });

        inventory.layout();
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Utility / Helpers
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    point2i32 window_to_world(point2i32 const p) const noexcept {
        auto const& tile_map = database.get_tile_map(tile_map_type::base);
        auto const tw = value_cast_unsafe<float>(tile_map.tile_width());
        auto const th = value_cast_unsafe<float>(tile_map.tile_height());

        auto const q  = current_view.window_to_world(p);
        auto const tx = floor_as<int32_t>(value_cast(q.x) / tw);
        auto const ty = floor_as<int32_t>(value_cast(q.y) / th);

        return {tx, ty};
    }

    // @param p Position in world coordinates
    void update_tile_at(point2i32 const p) {
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
          , region_id {}
        };

        renderer.update_map_data(lvl.update_tile_at(rng_superficial, p, data));
    }

    //! @param p Position in window coordinates
    void show_tool_tip(point2i32 const p) {
        auto const p0 = window_to_world(p);
        auto const q  = window_to_world({last_mouse_x, last_mouse_y});

        auto const was_visible = renderer.update_tool_tip_visible(true);
        renderer.update_tool_tip_position(p);

        if (was_visible && p0 == q) {
            return; // the tile the mouse points to is unchanged from last time
        }

        auto const& lvl  = the_world.current_level();
        auto const& tile = lvl.at(p0);

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
              , value_cast<int>(tile.rid)
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

        auto const id  = make_id<entity_id>("player");
        auto const def = database.find(id);
        BK_ASSERT(!!def);

        auto const result = add_entity_at(create_entity(*def), id, lvl.stair_up(0));
        BK_ASSERT(result.second == placement_result::ok);
    }

    void generate_entities() {
        auto& lvl = the_world.current_level();

        auto const id  = make_id<entity_id>("rat_small");
        auto const def = database.find(id);
        BK_ASSERT(!!def);

        auto ent = create_entity(*def);

        for (size_t i = 0; i < lvl.region_count(); ++i) {
            auto const& region = lvl.region(i);
            if (region.tile_count <= 0) {
                continue;
            }

            point2i32 const p {region.bounds.x0 + region.bounds.width()  / 2
                             , region.bounds.y0 + region.bounds.height() / 2};

            auto const result = add_entity_near(std::move(ent), id, p, 3, rng_substantive);
            if (result.second == placement_result::ok) {
                ent.reset(create_entity(*def).release());
            }
        }
    }

    void generate_items() {
        auto& lvl = the_world.current_level();

        auto const id  = make_id<item_id>("weapon_dagger");
        auto const def = database.find(id);
        BK_ASSERT(!!def);

        auto itm = create_item(*def);

        for (size_t i = 0; i < lvl.region_count(); ++i) {
            auto const& region = lvl.region(i);
            if (region.tile_count <= 0) {
                continue;
            }

            point2i32 const p {region.bounds.x0 + region.bounds.width()  / 2
                             , region.bounds.y0 + region.bounds.height() / 2};

            auto const result = add_item_near(std::move(itm), id, p, 3, rng_substantive);
            if (result.second == placement_result::ok) {
                itm.reset(create_item(*def).release());
            }
        }
    }

    void generate_level(level* const parent, size_t const id) {
        auto const level_w = 50;
        auto const level_h = 40;

        the_world.add_new_level(parent
          , make_level(rng_substantive, the_world, sizei32x {level_w}, sizei32y {level_h}, id));

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
        auto const tw = value_cast(tile_map.tile_width());
        auto const th = value_cast(tile_map.tile_height());

        auto const win_r = os.render_get_client_rect();
        auto const win_w = value_cast(win_r.width());
        auto const win_h = value_cast(win_r.height());

        auto const p  = get_player().second;
        auto const px = value_cast(p.x);
        auto const py = value_cast(p.y);

        current_view.x_off = static_cast<float>((win_w * 0.5) - tw * (px + 0.5));
        current_view.y_off = static_cast<float>((win_h * 0.5) - th * (py + 0.5));
        current_view.scale_x = 1.0f;
        current_view.scale_y = 1.0f;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // UI Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bool ui_on_key(kb_event const event, kb_modifiers const kmods) {
        return true;
    }

    bool ui_on_text_input(text_input_event const event) {
        return true;
    }

    bool ui_on_mouse_button(mouse_event const event, kb_modifiers const kmods) {
        if (!inventory.is_visible()) {
            return true;
        }

        if (event.button_state_bits() != 1
         || event.button_change[0] != mouse_event::button_change_t::went_down
        ) {
            return true;
        }

        auto const hit_test = inventory.hit_test({event.x, event.y});
        if (!hit_test) {
            return true;
        }

        using type = inventory_list::hit_test_result::type;
        static_string_buffer<128> buffer;

        switch (hit_test.what) {
        case type::none:  break;
        case type::empty: break;
        case type::header:
            buffer.append("Header (%d, %d)", hit_test.x, hit_test.y);
            break;
        case type::cell: {
            auto* const itm = the_world.find(inventory.row_data(hit_test.y));
            BK_ASSERT(!!itm);
            auto* const def = database.find(itm->definition());

            buffer.append("Cell (%d, %d) = %s"
              , hit_test.x
              , hit_test.y
              , def ? def->name.c_str() : "{undefined}"
            );

            break;
        }
        case type::title:
            buffer.append("Title (%d, %d)", hit_test.x, hit_test.y);
            break;
        case type::frame:
            buffer.append("Frame (%d, %d)", hit_test.x, hit_test.y);
            break;
        case type::button_close:
            buffer.append("Button (%d, %d)", hit_test.x, hit_test.y);
            break;
        case type::scroll_bar_v:
            buffer.append("Scroll V (%d, %d)", hit_test.x, hit_test.y);
            break;
        case type::scroll_bar_h:
            buffer.append("Scroll H (%d, %d)", hit_test.x, hit_test.y);
            break;
        default:
            BK_ASSERT(false);
            break;
        }

        if (!buffer.empty()) {
            message_window.println(buffer.to_string());
        }

        return false;
    }

    bool ui_on_mouse_move(mouse_event const event, kb_modifiers const kmods) {
        if (!inventory.is_visible()) {
            return true;
        }

        auto const hit_test = inventory.hit_test({last_mouse_x, last_mouse_y});
        if (!hit_test) {
            return true;
        }

        using type = inventory_list::hit_test_result::type;

        switch (hit_test.what) {
        case type::empty: break;
        case type::header:
            break;
        case type::cell:
            inventory.indicate(hit_test.y);
            break;
        case type::title:
            if (event.button_state_bits() != 1) {
                break;
            }

            inventory.move_by(point2i32 {event.x, event.y}
                            - point2i32 {last_mouse_x, last_mouse_y});
            break;
        case type::frame:
            if (event.button_state_bits() != 1) {
                break;
            }

            inventory.resize_by(
                event.x - last_mouse_x
              , event.y - last_mouse_y
              , hit_test.x
              , hit_test.y);

            break;
        case type::button_close:
            break;
        case type::scroll_bar_v:
            break;
        case type::scroll_bar_h:
            break;
        case type::none: BK_ATTRIBUTE_FALLTHROUGH;
        default:
            BK_ASSERT(false);
            break;
        }

        return false;
    }

    bool ui_on_mouse_wheel(int const wy, int const wx, kb_modifiers const kmods) {
        return true;
    }

    bool ui_on_command(command_type const type, uintptr_t const data) {
        if (!inventory.is_visible()) {
            return true;
        }

        auto const hit_test = inventory.hit_test({last_mouse_x, last_mouse_y});
        if (!hit_test) {
            return true;
        }

        if (type == command_type::move_n) {
            inventory.indicate_prev();
        } else if (type == command_type::move_s) {
            inventory.indicate_next();
        } else if (type == command_type::cancel) {
            inventory.hide();
        } else {
            return true;
        }

        return false;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //! @return true if events were not filtered, false otherwise.
    template <typename... Args0, typename... Args1>
    bool process_context_stack(
        event_result (input_context::* handler)(Args0...)
      , Args1&&... args
    ) {
        // as a stack: back to front
        for (auto i = context_stack.size(); i > 0; --i) {
            // size == 1 ~> index == 0
            auto const where = i - 1u;
            auto const r =
                (context_stack[where].*handler)(std::forward<Args1>(args)...);

            switch (r) {
            case event_result::filter_detach :
                context_stack.erase(
                    begin(context_stack) + static_cast<ptrdiff_t>(where));
                BK_ATTRIBUTE_FALLTHROUGH;
            case event_result::filter :
                return false;
            case event_result::pass_through_detach :
                context_stack.erase(
                    begin(context_stack) + static_cast<ptrdiff_t>(where));
                BK_ATTRIBUTE_FALLTHROUGH;
            case event_result::pass_through :
                break;
            default:
                BK_ASSERT(false);
                break;
            }
        }

        return true;
    }

    void on_key(kb_event const event, kb_modifiers const kmods) {
        // first, pass events to listeners
        if (!process_context_stack(&input_context::on_key, event, kmods)) {
            return;
        }

        if (event.went_down) {
            cmd_translator.translate(event);

            if (!kmods.test(kb_modifiers::m_shift)
               && (event.scancode == kb_scancode::k_lshift
                || event.scancode == kb_scancode::k_rshift)
            ) {
                show_tool_tip({last_mouse_x, last_mouse_y});
            }
        } else {
            if (!kmods.test(kb_modifiers::m_shift)) {
                renderer.update_tool_tip_visible(false);
            }
        }
    }

    void on_text_input(text_input_event const event) {
        // first, pass events to listeners
        if (!process_context_stack(&input_context::on_text_input, event)) {
            return;
        }

        cmd_translator.translate(event);
    }

    void on_mouse_button(mouse_event const event, kb_modifiers const kmods) {
        // first, allow ui a chance to filter events, then event listeners
        if (!ui_on_mouse_button(event, kmods)
         || !process_context_stack(&input_context::on_mouse_button, event, kmods)
        ) {
            return;
        }

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
        // always update the mouse position
        auto on_exit = BK_SCOPE_EXIT {
            last_mouse_x = event.x;
            last_mouse_y = event.y;
        };

        // first, allow ui a chance to filter events, then event listeners
        if (!ui_on_mouse_move(event, kmods)
         || !process_context_stack(&input_context::on_mouse_move, event, kmods)
        ) {
            return;
        }

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
    }

    void on_mouse_wheel(int const wy, int const wx, kb_modifiers const kmods) {
        // first, pass events to listeners
        if (!process_context_stack(&input_context::on_mouse_wheel, wy, wx, kmods)) {
            return;
        }

        auto const p_window = point2i32 {last_mouse_x, last_mouse_y};
        auto const p_world  = current_view.window_to_world(p_window);

        current_view.scale_x *= (wy > 0 ? 1.1f : 0.9f);
        current_view.scale_y  = current_view.scale_x;

        auto const p_window_new = current_view.world_to_window(p_world);

        current_view.x_off += value_cast_unsafe<float>(p_window.x) - value_cast(p_window_new.x);
        current_view.y_off += value_cast_unsafe<float>(p_window.y) - value_cast(p_window_new.y);
    }

    void on_command(command_type const type, uint64_t const data) {
        // first, allow ui a chance to filter events, then event listeners
        if (!ui_on_command(type, data)
         || !process_context_stack(&input_context::on_command, type, data)
        ) {
            return;
        }

        using ct = command_type;
        switch (type) {
        case ct::none : break;

        case ct::move_here : advance(1); break;

        case ct::move_n    : do_player_move_by({ 0, -1}); break;
        case ct::move_ne   : do_player_move_by({ 1, -1}); break;
        case ct::move_e    : do_player_move_by({ 1,  0}); break;
        case ct::move_se   : do_player_move_by({ 1,  1}); break;
        case ct::move_s    : do_player_move_by({ 0,  1}); break;
        case ct::move_sw   : do_player_move_by({-1,  1}); break;
        case ct::move_w    : do_player_move_by({-1,  0}); break;
        case ct::move_nw   : do_player_move_by({-1, -1}); break;

        case ct::move_down : do_change_level(ct::move_down); break;
        case ct::move_up   : do_change_level(ct::move_up);   break;

        case ct::get_all_items : do_get_all_items(); break;

        case ct::toggle_show_inventory : inventory.toggle_visible(); break;

        case ct::reset_view : reset_view_to_player(); break;
        case ct::reset_zoom:
            current_view.scale_x = 1.0f;
            current_view.scale_y = 1.0f;
            break;
        case ct::debug_toggle_regions :
            renderer.debug_toggle_show_regions();
            renderer.update_map_data();
            break;
        case ct::debug_teleport_self :
            do_debug_teleport_self();
            break;
        case ct::cancel :
            break;
        case ct::drop_one :
            do_drop_one();
            break;
        case ct::drop_some :
            do_drop_some();
            break;
        default:
            BK_ASSERT(false);
            break;
        }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Commands
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void renderer_update(entity_id const id, point2i32 const p_old, point2i32 const p_new) {
        entity_updates_.push_back({p_old, p_new, id});
    }

    void renderer_update(item_id const id, point2i32 const p_old, point2i32 const p_new) {
        item_updates_.push_back({p_old, p_new, id});
    }

    void renderer_update(entity const& e, point2i32 const p_old, point2i32 const p_new) {
        renderer_update(e.definition(), p_old, p_new);
    }

    void renderer_update(item const& i, point2i32 const p_old, point2i32 const p_new) {
        renderer_update(i.definition(), p_old, p_new);
    }

    void renderer_add(entity_id const id, point2i32 const p) {
        renderer_update(id, p, p);
    }

    void renderer_add(item_id const id, point2i32 const p) {
        renderer_update(id, p, p);
    }

    void renderer_remove_item(point2i32 const p) {
        item_updates_.push_back({p, p, item_id {}});
    }

    void renderer_remove_entity(point2i32 const p) {
        entity_updates_.push_back({p, p, entity_id {}});
    }

    void do_drop_one() {
        inventory.show();
    }

    void do_drop_some() {
    }

    void do_debug_teleport_self() {
        message_window.println("Teleport where?");

        input_context c;

        c.on_mouse_button_handler =
            [&](mouse_event const event, kb_modifiers const kmods) {
                if (event.button_state_bits() != 1u) {
                    return event_result::filter;
                }

                auto const result =
                    do_player_move_to(window_to_world({event.x, event.y}));

                if (result != placement_result::ok) {
                    message_window.println("Invalid destination. Choose another.");
                    return event_result::filter;
                }

                return event_result::filter_detach;
            };

        c.on_command_handler =
            [&](command_type const type, uint64_t) {
                if (type == command_type::debug_teleport_self) {
                    message_window.println("Already teleporting.");
                    return event_result::filter;
                } else if (type == command_type::cancel) {
                    message_window.println("Canceled teleporting.");
                    return event_result::filter_detach;
                }

                return event_result::pass_through;
            };

        context_stack.push_back(std::move(c));
    }

    void do_get_all_items() {
        auto const player_info = get_player();
        auto&      player      = player_info.first;
        auto const p           = player_info.second;
        auto&      lvl         = the_world.current_level();

        // lookup an item from its instance id
        auto const get_item = [&](item_instance_id const id) noexcept -> item const& {
            auto* const itm = the_world.find(id);
            BK_ASSERT(!!itm);
            return *itm;
        };

        // get the (possibly undefined) name for an item
        auto const get_item_name = [&](item const& itm) noexcept {
            auto* const def = database.find(itm.definition());
            return def
              ? def->name.c_str()
              : "{undefined}";
        };

        static_string_buffer<128> buffer;

        auto const pickup_item = [&](item_instance_id const id) noexcept {
            auto const& itm  = get_item(id);

            buffer.clear();
            buffer.append("Picked up %s.", get_item_name(itm));

            message_window.println(buffer.to_string());

            inventory.add_row(id); // TODO temp

            return item_merge_result::ok;
        };

        auto const result = lvl.move_items(p, player, pickup_item);

        switch (result) {
        case merge_item_result::ok_merged_none:
            break;
        case merge_item_result::ok_merged_some:
        case merge_item_result::ok_merged_all:
            renderer_remove_item(p);
            inventory.layout(); // TODO temp
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

    void do_combat(point2i32 const att_pos, point2i32 const def_pos) {
        auto& lvl = the_world.current_level();

        auto* const att = lvl.entity_at(att_pos);
        auto* const def = lvl.entity_at(def_pos);

        BK_ASSERT(!!att && !!def);
        BK_ASSERT(att->is_alive() && def->is_alive());

        def->modify_health(-1);

        if (!def->is_alive()) {
            auto const id = [&] {
                auto const n = random_weighted(rng_superficial, weight_list<int, int> {
                    {5,  0} // 0~5 -> 6/10
                  , {9,  1} // 6-9 -> 3/10
                  , {10, 2} // 10  -> 1/10
                });

                if (n == 0) {
                    return item_id {};
                } else if (n == 1) {
                    return make_id<item_id>("coin");
                }

                BK_ASSERT(n == 2);

                return make_id<item_id>("potion_health_small");
            }();

            if (value_cast(id)) {
                auto const idef = database.find(id);
                BK_ASSERT(!!idef);
                add_item_at(create_item(*idef), id, def_pos);
            }

            lvl.remove_entity(def->instance());
            renderer_remove_entity(def_pos);
        }

        advance(1);
    }

    //! Attempt to change levels at the current player position.
    //! @param type Must be either command_type::move_down or
    //! command_type::move_up.
    void do_change_level(command_type const type) {
        BK_ASSERT(type == command_type::move_down
               || type == command_type::move_up);

        auto& cur_lvl = the_world.current_level();

        auto const player          = get_player();
        auto const player_p        = player.second;
        auto const player_instance = player.first.instance();
        auto const player_id       = player.first.definition();

        auto const delta = [&]() noexcept {
            auto const tile = cur_lvl.at(player_p);

            auto const tile_code = (tile.id == tile_id::stair_down)  ? 1
                                 : (tile.id == tile_id::stair_up)    ? 2
                                                                     : 0;
            auto const move_code = (type == command_type::move_down) ? 1
                                 : (type == command_type::move_up)   ? 2
                                                                     : 0;
            switch ((move_code << 2) | tile_code) {
            case 0b0100 : // move_down & other
            case 0b1000 : // move_up   & other
                message_window.println("There are no stairs here.");
                break;;
            case 0b0101 : // move_down & stair_down
                return 1;
            case 0b1010 : // move_up   & stair_up
                return -1;
            case 0b0110 : // move_down & stair_up
                message_window.println("You can't go down here.");
                break;
            case 0b1001 : // move_up   & stair_down
                message_window.println("You can't go up here.");
                break;
            default:
                BK_ASSERT(false); // some other command was given
                break;
            }

            return 0;
        }();

        if (delta == 0) {
            return;
        }

        auto const id = static_cast<ptrdiff_t>(cur_lvl.id());
        if (id + delta < 0) {
            message_window.println("You can't leave.");
            return;
        }

        auto const next_id = static_cast<size_t>(id + delta);

        auto player_ent = cur_lvl.remove_entity(player_instance);

        if (!the_world.has_level(next_id)) {
            generate(next_id);
        } else {
            set_current_level(next_id);
        }

        // the level has been changed at this point

        auto const p = (delta > 0)
          ? the_world.current_level().stair_up(0)
          : the_world.current_level().stair_down(0);

        add_entity_near(std::move(player_ent), player_id, p, 5, rng_substantive);

        reset_view_to_player();
    }

    placement_result do_player_move_to(point2i32 const p) {
        auto& lvl = the_world.current_level();

        auto const player_info = get_player();
        auto&      player      = player_info.first;

        auto const p_cur = player_info.second;
        auto const p_dst = p;

        auto const result = lvl.move_by(player.instance(), p_dst - p_cur);

        switch (result) {
        case placement_result::ok:
            renderer_update(player, p_cur, p_dst);
            break;
        case placement_result::failed_entity:   BK_ATTRIBUTE_FALLTHROUGH;
        case placement_result::failed_obstacle: BK_ATTRIBUTE_FALLTHROUGH;
        case placement_result::failed_bounds:   BK_ATTRIBUTE_FALLTHROUGH;
        case placement_result::failed_bad_id:
            break;
        default:
            BK_ASSERT(false);
            break;
        }

        return result;
    }

    placement_result do_player_move_by(vec2i32 const v) {
        auto& lvl = the_world.current_level();

        auto const player_info = get_player();
        auto&      player      = player_info.first;

        auto const p_cur = player_info.second;
        auto const p_dst = p_cur + v;

        auto const result = lvl.move_by(player.instance(), v);

        switch (result) {
        case placement_result::ok:
            renderer_update(player, p_cur, p_dst);
            advance(1);
            break;
        case placement_result::failed_entity:
            do_combat(p_cur, p_dst);
            break;
        case placement_result::failed_obstacle:
            interact_obstacle(player, p_cur, p_dst);
            break;
        case placement_result::failed_bounds: BK_ATTRIBUTE_FALLTHROUGH;
        case placement_result::failed_bad_id:
            break;
        default :
            BK_ASSERT(false);
            break;
        }

        return result;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Simulation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    std::pair<entity const&, point2i32> get_player() const {
        return const_cast<game_state*>(this)->get_player();
    }

    std::pair<entity&, point2i32> get_player() {
        auto const result = the_world.current_level().find(entity_instance_id {1u});
        BK_ASSERT(result.first);
        return {*result.first, result.second};
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

    using placement_pair = std::pair<point2i32, placement_result>;

    placement_pair add_item_near(unique_item&& i, item_id const id, point2i32 const p, int32_t const distance, random_state& rng) {
        auto const result = the_world.current_level()
          .add_item_nearest_random(rng, std::move(i), p, distance);

        if (result.second == placement_result::ok) {
            renderer_add(id, p);
        }

        return result;
    }

    placement_pair add_entity_near(unique_entity&& e, entity_id const id, point2i32 const p, int32_t const distance, random_state& rng) {
        auto const result = the_world.current_level()
          .add_entity_nearest_random(rng, std::move(e), p, distance);

        if (result.second == placement_result::ok) {
            renderer_add(id, p);
        }

        return result;
    }

    placement_pair add_item_at(unique_item&& i, item_id const id, point2i32 const p) {
        auto const result =
            the_world.current_level().add_item_at(std::move(i), p);

        if (result == placement_result::ok) {
            renderer_add(id, p);
        }

        return {p, result};
    }

    placement_pair add_entity_at(unique_entity&& e, entity_id const id, point2i32 const p) {
        auto const result =
            the_world.current_level().add_entity_at(std::move(e), p);

        if (result == placement_result::ok) {
            renderer_add(id, p);
        }

        return {p, result};
    }

    void interact_obstacle(entity& e, point2i32 const cur_pos, point2i32 const obstacle_pos) {
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
                , region_id {}
            };

            renderer.update_map_data(lvl.update_tile_at(rng_superficial, obstacle_pos, data));
        }
    }

    //! Advance the game time by @p steps
    void advance(int const steps) {
        auto& lvl = the_world.current_level();

        lvl.transform_entities(
            [&](entity& e, point2i32 const p) noexcept {
                // the player
                if (e.instance() == entity_instance_id {1u}) {
                    return p;
                }

                if (!random_chance_in_x(rng_superficial, 1, 10)) {
                    return p;
                }

                constexpr std::array<int, 4> dir_x {-1,  0, 0, 1};
                constexpr std::array<int, 4> dir_y { 0, -1, 1, 0};

                auto const dir = static_cast<size_t>(random_uniform_int(rng_superficial, 0, 3));
                auto const d   = vec2i32 {dir_x[dir], dir_y[dir]};

                return p + d;
            }
          , [&](entity& e, point2i32 const p, point2i32 const q) noexcept {
                entity_updates_.push_back({p, q, e.definition()});
            }
        );
    }

    //! Render the game
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

    //! The main game loop
    void run() {
        while (os.is_running()) {
            os.do_events();
            render(last_frame_time);
        }
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

        inventory_list::lookup_f inventory_lookup() const noexcept {
            return [&](item_instance_id const id) noexcept -> item const& {
                auto const ptr = world_ptr->find(id);
                BK_ASSERT(!!ptr);
                return *ptr;
            };
        }

        up<inventory_list> inventory_ptr = make_inventory_list(*trender_ptr, inventory_lookup());
    } state {};

    system&             os              = *state.system_ptr;
    random_state&       rng_substantive = *state.rng_substantive_ptr;
    random_state&       rng_superficial = *state.rng_superficial_ptr;
    game_database&      database        = *state.database_ptr;
    world&              the_world       = *state.world_ptr;
    game_renderer&      renderer        = *state.renderer_ptr;
    text_renderer&      trender         = *state.trender_ptr;
    command_translator& cmd_translator  = *state.cmd_translator_ptr;
    inventory_list&     inventory       = *state.inventory_ptr;

    std::vector<input_context> context_stack;

    view current_view;

    int last_mouse_x = 0;
    int last_mouse_y = 0;

    std::vector<game_renderer::update_t<item_id>>   item_updates_;
    std::vector<game_renderer::update_t<entity_id>> entity_updates_;

    timepoint_t last_frame_time {};

    message_log message_window {trender};
};

} // namespace boken

namespace {
#if defined(BK_NO_TESTS)
void run_tests() {
}
#else
void run_tests() {
    using namespace std::chrono;

    auto const beg = high_resolution_clock::now();
    boken::run_unit_tests();
    auto const end = high_resolution_clock::now();

    std::printf("Tests took %" PRId64 " microseconds.\n",
        duration_cast<microseconds>(end - beg).count());
}
#endif // BK_NO_TESTS
} // namespace

int main(int const argc, char const* argv[]) try {
    run_tests();

    bk::game_state game;
    game.run();

    return 0;
} catch (...) {
    return 1;
}
