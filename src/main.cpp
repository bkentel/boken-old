#include "catch.hpp"        // for run_unit_tests
#include "algorithm.hpp"
#include "allocator.hpp"
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
#include "item_properties.hpp"
#include "timer.hpp"
#include "item_list.hpp"

#include <algorithm>        // for move
#include <chrono>           // for microseconds, operator-, duration, etc
#include <deque>
#include <functional>       // for function
#include <memory>           // for unique_ptr, allocator
#include <ratio>            // for ratio
#include <string>           // for string, to_string
#include <utility>          // for pair, make_pair
#include <vector>           // for vector

#include <cstdint>          // for uint16_t, uint32_t, uintptr_t
#include <cstdio>           // for printf
#include <cinttypes>

namespace boken {

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int get_entity_loot(entity& e, random_state& rng, std::function<void(unique_item&&)> const& f) {
    int result = 0;

    auto& items = e.items();
    while (!items.empty()) {
        auto itm = items.remove_item(0);
        f(std::move(itm));
        ++result;
    }

    return result;
}

//! Game input sink
class input_context {
public:
    input_context() = default;
    input_context(char const* const name)
      : debug_name {name}
    {
    }

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

    char const* debug_name = "{anonymous}";
};

class input_context_stack {
public:
    using id_t = uint32_t;

    size_t size() const noexcept {
        return contexts_.size();
    }

    id_t push(input_context context) {
        auto const id = get_next_id_();
        contexts_.push_back({std::move(context), id});
        return id;
    }

    void pop(id_t const id) {
        BK_ASSERT(!contexts_.empty());

        auto const first = contexts_.rbegin();
        auto const last  = contexts_.rend();

        auto const it = std::find_if(first, last
          , [id](pair_t const& p) noexcept {
                return id == p.second;
            });

        BK_ASSERT((it != last) && (it->second == id));

        if (id == next_id_ - 1) {
            --next_id_;
        } else {
            free_ids_.push_back(id);
        }

        contexts_.erase(std::next(it).base());
    }

    //! @returns true if the event has not been filtered, false otherwise.
    template <typename... Args0, typename... Args1>
    bool process(
        event_result (input_context::* handler)(Args0...)
      , Args1&&... args
    ) {
        // as a stack: back to front
        for (auto i = size(); i > 0; --i) {
            auto const where   = i - 1u; // size == 1 ~> index == 0
            auto&      context = contexts_[where].first;
            auto const id      = contexts_[where].second;

            auto const result =
                (context.*handler)(std::forward<Args1>(args)...);

            switch (result) {
            case event_result::filter_detach :
                pop(id);
                BK_ATTRIBUTE_FALLTHROUGH;
            case event_result::filter :
                return false;
            case event_result::pass_through_detach :
                pop(id);
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
private:
    using pair_t = std::pair<input_context, id_t>;

    id_t get_next_id_() {
        if (!free_ids_.empty()) {
            auto const result = free_ids_.front();
            free_ids_.pop_front();
            return result;
        }

        return ++next_id_;
    }

    std::deque<id_t>    free_ids_;
    std::vector<pair_t> contexts_;
    id_t                next_id_ {};
};

struct game_state {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Convenience functions
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    auto item_list_index_to_id() noexcept {
        return [&](int const i) noexcept {
            return item_list.get().row_data(i);
        };
    }

    //--------------------------------------------------------------------------
    // Player functions
    //--------------------------------------------------------------------------
    std::pair<entity&, point2i32> get_player() noexcept {
        constexpr auto player_id = entity_instance_id {1u};

        auto const result = the_world.current_level().find(player_id);
        BK_ASSERT(!!result.first);

        return {*result.first, result.second};
    }

    std::pair<entity const&, point2i32> get_player() const noexcept {
        return const_cast<game_state*>(this)->get_player();
    }

    static constexpr entity_instance_id player_id() noexcept {
        return entity_instance_id {1u};
    }

    point2i32 player_location() const noexcept {
        auto const p = current_level().find_position(player_id());
        BK_ASSERT(p.first);
        return p.second;
    }

    //--------------------------------------------------------------------------
    // Level functions
    //--------------------------------------------------------------------------
    level& current_level() noexcept {
        return the_world.current_level();
    }

    level const& current_level() const noexcept {
        return the_world.current_level();
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    item_id get_pile_id() const {
        return boken::get_pile_id(database);
    }

    item_id get_pile_id(item_pile const& pile, item_id const pile_id) const {
        return boken::get_pile_id(ctx, pile, pile_id);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Types
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    using clock_t     = std::chrono::high_resolution_clock;
    using timepoint_t = clock_t::time_point;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Special member functions
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    std::unique_ptr<inventory_list> make_item_list_() {
        return make_inventory_list(ctx, trender);
    }

    void set_item_list_columns() {
        item_list.add_column("", [&](const_item_descriptor const i) {
            auto const& tmap = database.get_tile_map(tile_map_type::item);
            auto const index = tmap.find(i.obj.definition());

            BK_ASSERT(index < 0x7Fu); //TODO

            std::array<char, 7> as_string {};
            as_string[0] =
                static_cast<char>(static_cast<uint8_t>(index & 0x7Fu));

            return std::string {as_string.data()};
        });

        item_list.add_column("Name", [&](const_item_descriptor const i) {
            return name_of_decorated(ctx, i);
        });

        item_list.add_column("Weight", [&](const_item_descriptor const i) {
            return std::to_string(weight_of_inclusive(ctx, i));
        });

        item_list.add_column("Count", [&](const_item_descriptor const i) {
            auto const stack = get_property_value_or(
                i, property(item_property::current_stack_size), 1);

            return std::to_string(stack);
        });

        item_list.layout();
    }

    game_state()
      : item_list {make_item_list_()}
    {
        bind_event_handlers_();

        renderer.set_message_window(&message_window);

        renderer.set_tile_maps({
            {tile_map_type::base,   database.get_tile_map(tile_map_type::base)}
          , {tile_map_type::entity, database.get_tile_map(tile_map_type::entity)}
          , {tile_map_type::item,   database.get_tile_map(tile_map_type::item)}
        });

        renderer.set_inventory_window(&item_list.get());

        generate();

        reset_view_to_player();

        set_item_list_columns();

        item_list.hide();

        item_list.set_on_focus_change([&](bool const is_focused) {
            renderer.set_inventory_window_focus(is_focused);
            if (is_focused) {
                renderer.update_tool_tip_visible(false);
            }
        });
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Utility / Helpers
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    point2i32 window_to_world(point2i32 const p) const noexcept {
        auto const& tmap = database.get_tile_map(tile_map_type::base);

        return underlying_cast_unsafe<int32_t>(
            floor(current_view.window_to_world(
                p, tmap.tile_width(), tmap.tile_height())));
    }

    point2i32 world_to_window(point2i32 const p) const noexcept {
        auto const& tmap = database.get_tile_map(tile_map_type::base);

        //TODO: round?
        return underlying_cast_unsafe<int32_t>(current_view.world_to_window(
            p, tmap.tile_width(), tmap.tile_height()));
    }

    //! Update the rendering scale.
    //! @pre sx > 0 && sy > 0
    void update_view_scale(float const sx, float const sy) noexcept {
        update_view(sx, sy, current_view.x_off, current_view.y_off);
    }

    void update_view_scale(point2f const scale) noexcept {
        update_view_scale(value_cast(scale.x), value_cast(scale.y));
    }

    //! Update the rendering offset (translation).
    void update_view_trans(float const dx, float const dy) {
        update_view(current_view.scale_x, current_view.scale_y, dx, dy);
    }

    void update_view_trans(point2f const trans) noexcept {
        update_view_trans(value_cast(trans.x), value_cast(trans.y));
    }

    //! Update the rendering transformation matrix.
    //! @pre sx > 0 && sy > 0
    void update_view(float const sx, float const sy
                   , float const dx, float const dy
    ) noexcept {
        BK_ASSERT(sx > 0.0f && sy > 0.0f);

        current_view.scale_x = sx;
        current_view.scale_y = sy;
        current_view.x_off   = dx;
        current_view.y_off   = dy;

        update_highlight_tile();
    }

    void update_view(point2f const scale, point2f const trans) noexcept {
        update_view(value_cast(scale.x), value_cast(scale.y)
                  , value_cast(trans.x), value_cast(trans.y));
    }

    //! Debug command to create a corridor at the give position.
    //! @param p Position in world coordinates
    void debug_create_corridor_at(point2i32 const p) {
        if (!intersects(p, the_world.current_level().bounds())) {
            return;
        }

        tile_data_set const data {
            tile_data {}
          , tile_flags {0}
          , tile_id::tunnel
          , tile_type::tunnel
          , region_id {}
        };

        update_tile_at(p, data);
    }

    //! Change the properties of the tile for the current level at the given
    //! position.
    //! @param p Position in world coordinates
    void update_tile_at(point2i32 const p, tile_data_set const& data) {
        auto& lvl = the_world.current_level();

        BK_ASSERT(intersects(lvl.bounds(), p));

        renderer.update_map_data(lvl.update_tile_at(rng_superficial, p, data));
    }

    //! Show the toolip for the 'view' command
    //! @param p Position in world coordinates
    void show_view_tool_tip(point2i32 const p) {
        auto const& lvl  = the_world.current_level();
        auto const& tile = lvl.at(p);

        static_string_buffer<256> buffer;

        auto const print_entity = [&]() noexcept {
            auto* const entity = lvl.entity_at(p);
            if (!entity) {
                return true;
            }

            auto const e = const_entity_descriptor {database, *entity};

            return buffer.append("%s\n", name_of(ctx, e).data());
        };

        auto const print_items = [&]() noexcept {
            auto* const pile = lvl.item_at(p);
            if (!pile) {
                return true;
            }

            auto const size = pile->size();
            BK_ASSERT(size >= 1u);

            size_t i = 0;
            for (auto const& id : *pile) {
                auto const sep = (i++ == size - 1) ? "" : ", ";
                auto const itm = const_item_descriptor {ctx, id};

                auto const result = buffer.append("%s%s"
                  , name_of_decorated(ctx, itm).data(), sep);

                if (!result) {
                    return false;
                }
            }

            return buffer.append("\n");
        };

        auto const result =
            buffer.append("You see here: %s\n"
              , enum_to_string(lvl.at(p).id).data())
         && print_entity()
         && print_items();

        renderer.update_tool_tip_text(buffer.to_string());
    }

    //! @param p Position in window coordinates
    void debug_show_tool_tip(point2i32 const p) {
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

                auto const& itm = the_world.find(id);
                auto* const def = database.find(itm.definition());

                buffer.append(
                    " Instance  : %0#10x\n"
                    " Definition: %0#10x (%s)\n"
                    " Name      : %s\n"
                  , value_cast(itm.instance())
                  , value_cast(itm.definition()), (def ? def->id_string.c_str() : "{empty}")
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
        auto const result = create_object_at(
            make_id<entity_id>("player")
          , the_world.current_level().stair_up(0)
          , rng_substantive);

        BK_ASSERT(result.second);
    }

    void generate_entities() {
        weight_list<int, item_id> const w {
            {6, item_id {}}
          , {3, make_id<item_id>("coin")}
          , {1, make_id<item_id>("potion_health_small")}
        };

        auto const w_max = w.max();

        auto& rng = rng_substantive;
        auto& lvl = the_world.current_level();

        auto const def_ptr = database.find(make_id<entity_id>("rat_small"));
        BK_ASSERT(!!def_ptr);
        auto const& def = *def_ptr;

        for (size_t i = 0; i < lvl.region_count(); ++i) {
            auto const& region = lvl.region(i);
            if (region.tile_count <= 0) {
                continue;
            }

            point2i32 const p {region.bounds.x0 + region.bounds.width()  / 2
                             , region.bounds.y0 + region.bounds.height() / 2};

            auto const result =
                lvl.find_valid_entity_placement_neareast(rng, p, 3);

            if (result.second != placement_result::ok) {
                continue;
            }

            auto const instance_id = create_object_at(def, result.first, rng);
            auto& new_entity = find(the_world, instance_id);

            auto const id = random_weighted(rng, w);
            if (id == item_id {}) {
                continue;
            }

            auto* const idef = database.find(id);
            if (!idef) {
                BK_ASSERT(false);
                continue; //TODO
            }

            new_entity.add_item(create_object(*idef, rng));
        }
    }

    void generate_items() {
        auto& lvl = the_world.current_level();
        auto& rng = rng_substantive;

        auto const def_ptr = database.find(make_id<item_id>("container_chest"));
        BK_ASSERT(!!def_ptr);
        auto const& def = *def_ptr;

        auto const dag_id = make_id<item_id>("weapon_dagger");
        auto const dag_def_ptr = database.find(dag_id);
        BK_ASSERT(!!dag_def_ptr);
        auto const& dag_def = *dag_def_ptr;

        for (size_t i = 0; i < lvl.region_count(); ++i) {
            auto const& region = lvl.region(i);
            if (region.tile_count <= 0) {
                continue;
            }

            point2i32 const p {region.bounds.x0 + region.bounds.width()  / 2
                             , region.bounds.y0 + region.bounds.height() / 2};

            auto const result =
                lvl.find_valid_item_placement_neareast(rng, p, 3);

            if (result.second != placement_result::ok) {
                continue;
            }

            auto const container_id =
                create_object_at(def, result.first, rng);

            auto const dagger_id =
                create_object_at(dag_def, result.first, rng);

            auto const container = item_descriptor {ctx, container_id};
            auto const dagger    = item_descriptor {ctx, dagger_id};

            if (can_add_item(ctx, dagger, container, ignore {})) {
                lvl.move_items(result.first, [&](unique_item&& itm) {
                    if (itm.get() != dagger.obj.instance()) {
                        return;
                    }

                    merge_into_pile(ctx, std::move(itm), dagger, container);
                });
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
        BK_ASSERT(!the_world.has_level(id));

        if (id == 0) {
            generate_level(nullptr, id);
            generate_player();
        } else {
            generate_level(&the_world.current_level(), id);
        }

        generate_entities();
        generate_items();

        set_current_level(id, true);
    }

    void set_current_level(size_t const level_id, bool const is_new) {
        BK_ASSERT(the_world.has_level(level_id));
        renderer.set_level(the_world.change_level(level_id));

        renderer.update_map_data();

        if (is_new) {
            return;
        }

        item_updates_.clear();
        entity_updates_.clear();

        auto& lvl = the_world.current_level();

        lvl.for_each_entity([&](entity_instance_id const id, point2i32 const p) {
            renderer_add(find(the_world, id).definition(), p);
        });

        auto const pile_id = get_pile_id();

        lvl.for_each_pile([&](item_pile const& pile, point2i32 const p) {
            renderer_add(get_pile_id(pile, pile_id), p);
        });
    }

    void reset_view_to_player() {
        auto const& tmap  = database.get_tile_map(tile_map_type::base);
        auto const  win_r = os.render_get_client_rect();

        auto const q = current_view.center_window_on_world(
            get_player().second, tmap.tile_width(), tmap.tile_height()
          , win_r.width(), win_r.height());

        update_view({1.0f, 1.0f}, q);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Events
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //! @returns true if the event has not been filtered, false otherwise.
    template <typename... Args0, typename... Args1>
    void process_event(
        bool (game_state::* ui_handler)(Args0...)
      , event_result (input_context::* ctx_handler)(Args0...)
      , void (game_state::* base_handler)(Args0...)
      , Args1&&... args
    ) {
        // first allow the ui a chance to process the event
        if (!(this->*ui_handler)(std::forward<Args1>(args)...)) {
            return; // ui filtered the event
        }

        // then allow the input contexts a chance to process the event
        if (!context_stack.process(ctx_handler, std::forward<Args1>(args)...)) {
            return; // an input context filtered the event
        }

        // lastly, allow the default handler to process the event
        (this->*base_handler)(std::forward<Args1>(args)...);
    }

    void bind_event_handlers_() {
        os.on_key([&](kb_event const event, kb_modifiers const kmods) {
            process_event(&game_state::ui_on_key
                        , &input_context::on_key
                        , &game_state::on_key
                        , event, kmods);
            cmd_translator.translate(event, kmods);
        });

        os.on_text_input([&](text_input_event const event) {
            process_event(&game_state::ui_on_text_input
                        , &input_context::on_text_input
                        , &game_state::on_text_input
                        , event);
            cmd_translator.translate(event);
        });

        os.on_mouse_move([&](mouse_event const event, kb_modifiers const kmods) {
            process_event(&game_state::ui_on_mouse_move
                        , &input_context::on_mouse_move
                        , &game_state::on_mouse_move
                        , event, kmods);

            last_mouse_x = event.x;
            last_mouse_y = event.y;
        });

        os.on_mouse_button([&](mouse_event const event, kb_modifiers const kmods) {
            process_event(&game_state::ui_on_mouse_button
                        , &input_context::on_mouse_button
                        , &game_state::on_mouse_button
                        , event, kmods);
        });

        os.on_mouse_wheel([&](int const wx, int const wy, kb_modifiers const kmods) {
            process_event(&game_state::ui_on_mouse_wheel
                        , &input_context::on_mouse_wheel
                        , &game_state::on_mouse_wheel
                        , wx, wy, kmods);
        });

        cmd_translator.on_command([&](command_type const type, uint64_t const data) {
            process_event(&game_state::ui_on_command
                        , &input_context::on_command
                        , &game_state::on_command
                        , type, data);
        });
    }

    bool ui_on_key(kb_event const event, kb_modifiers const kmods) {
        return item_list.on_key(event, kmods);
    }

    bool ui_on_text_input(text_input_event const event) {
        return item_list.on_text_input(event);
    }

    bool ui_on_mouse_button(mouse_event const event, kb_modifiers const kmods) {
        return item_list.on_mouse_button(event, kmods);
    }

    bool ui_on_mouse_move(mouse_event const event, kb_modifiers const kmods) {
        return item_list.on_mouse_move(event, kmods);
    }

    bool ui_on_mouse_wheel(int const wy, int const wx, kb_modifiers const kmods) {
        return item_list.on_mouse_wheel(wy, wx, kmods);
    }

    bool ui_on_command(command_type const type, uintptr_t const data) {
        return item_list.on_command(type, data);
    }

    void on_key(kb_event const event, kb_modifiers const kmods) {
        auto const is_shift =
            (!kmods.any(kb_mod::shift))
         && ((event.scancode == kb_scancode::k_lshift)
          || (event.scancode == kb_scancode::k_rshift));

        if (is_shift&& !event.went_down) {
            if (highlighted_tile == point2i32 {-1, -1}) {
                renderer.update_tool_tip_visible(false);
            } else {
                update_highlighted_tile({});
            }
        }
    }

    void on_text_input(text_input_event const event) {
    }

    void on_mouse_button(mouse_event const event, kb_modifiers const kmods) {
        using mbc = mouse_event::button_change_t;

        switch (event.button_state_bits()) {
        case 0b0000 :
            // no buttons down
            break;
        case 0b0001 :
            // left mouse button only

            if (event.button_change[0] == mbc::went_down) {
                if (kmods.exclusive_any(kb_mod::alt)) {
                    debug_create_corridor_at(
                        window_to_world({event.x, event.y}));
                }
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
            if (kmods.exclusive_any(kb_mod::shift)) {
                debug_show_tool_tip({event.x, event.y});
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
                update_view_trans(
                    current_view.x_off + static_cast<float>(event.dx)
                  , current_view.y_off + static_cast<float>(event.dy));
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
        auto const p_window = point2i32 {last_mouse_x, last_mouse_y};
        auto const p_world  = current_view.window_to_world(p_window);

        auto const scale = current_view.scale_x * (wy > 0 ? 1.1f : 0.9f);
        update_view_scale(scale, scale);

        auto const p_window_new = current_view.world_to_window(p_world);

        auto const dx = current_view.x_off
            + value_cast_unsafe<float>(p_window.x) - value_cast(p_window_new.x);
        auto const dy = current_view.y_off
            + value_cast_unsafe<float>(p_window.y) - value_cast(p_window_new.y);

        update_view_trans(dx, dy);
    }

    void on_command(command_type const type, uint64_t const data) {
        using ct = command_type;
        switch (type) {
        case ct::none : break;

        case ct::move_here : advance(1); break;

        case ct::move_n  : do_player_move_by({ 0, -1}); break;
        case ct::move_ne : do_player_move_by({ 1, -1}); break;
        case ct::move_e  : do_player_move_by({ 1,  0}); break;
        case ct::move_se : do_player_move_by({ 1,  1}); break;
        case ct::move_s  : do_player_move_by({ 0,  1}); break;
        case ct::move_sw : do_player_move_by({-1,  1}); break;
        case ct::move_w  : do_player_move_by({-1,  0}); break;
        case ct::move_nw : do_player_move_by({-1, -1}); break;

        case ct::run_n  : do_player_run({ 0, -1}); break;
        case ct::run_ne : do_player_run({ 1, -1}); break;
        case ct::run_e  : do_player_run({ 1,  0}); break;
        case ct::run_se : do_player_run({ 1,  1}); break;
        case ct::run_s  : do_player_run({ 0,  1}); break;
        case ct::run_sw : do_player_run({-1,  1}); break;
        case ct::run_w  : do_player_run({-1,  0}); break;
        case ct::run_nw : do_player_run({-1, -1}); break;

        case ct::move_down : do_change_level(ct::move_down); break;
        case ct::move_up   : do_change_level(ct::move_up);   break;

        case ct::get_all_items : do_get_all_items(); break;
        case ct::get_items : do_get_items(); break;

        case ct::toggle_show_inventory : do_toggle_inventory(); break;

        case ct::reset_view : reset_view_to_player(); break;
        case ct::reset_zoom:
            BK_ASSERT(false); // TODO
            break;
        case ct::debug_toggle_regions :
            renderer.debug_toggle_show_regions();
            renderer.update_map_data();
            break;
        case ct::debug_teleport_self : do_debug_teleport_self(); break;
        case ct::cancel : do_cancel(); break;
        case ct::confirm : break;
        case ct::toggle : break;
        case ct::drop_one : do_drop_one(); break;
        case ct::drop_some : do_drop_some(); break;
        case ct::open : do_open(); break;
        case ct::view : do_view(); break;

        case ct::alt_get_items : break;
        case ct::alt_drop_some : break;
        case ct::alt_open : break;
        case ct::alt_insert : break;

        default:
            BK_ASSERT(false);
            break;
        }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Helpers
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //! updates the window space position of the tooltip associated with the
    //! view command.
    void update_highlight_tile() {
        auto const p = highlighted_tile;
        if (p == point2i32 {-1, -1}) {
            return;
        }

        auto const q = world_to_window(p + vec2i32 {1, 0});
        renderer.update_tool_tip_position(q);
    }

    void set_highlighted_tile(point2i32 const p) {
        auto const bounds = the_world.current_level().bounds();
        auto const q      = clamp(bounds, p);

        highlighted_tile = q;
        renderer.set_tile_highlight(q);
        show_view_tool_tip(q);

        update_highlight_tile();
        renderer.update_tool_tip_visible(true);

        adjust_view_to_player(q);
    }

    void update_highlighted_tile(vec2i32 const v) {
        set_highlighted_tile(highlighted_tile + v);
    }

    template <typename OnCommand>
    void impl_choose_items_(
        int const     n
      , std::string&& title
      , OnCommand     on_command
    ) {
        item_list.set_title(std::move(title));
        item_list.set_modal(true);
        item_list.set_multiselect(n > 1);
        item_list.show();

        item_list.set_on_command([=](command_type const cmd) {
            return on_command(cmd);
        });
    }

    template <typename OnCommand>
    void choose_multiple_items(std::string title, OnCommand on_command) {
        impl_choose_items_(2, std::move(title), on_command);
    }

    template <typename OnCommand>
    void choose_single_item(std::string title, OnCommand on_command) {
        impl_choose_items_(1, std::move(title), on_command);
    }

    //! hard fail if the entity doesn't exists on the given level.
    point2i32 require_entity_on_level(
        const_entity_descriptor const  e
      , level                   const& lvl
    ) const {
        auto const p = lvl.find(e.obj.instance());
        BK_ASSERT_SAFE(p.first);
        return p.second;
    }

    int drop_items(
        entity_descriptor const subject
      , point2i32         const to_p
      , int const*        const first = nullptr
      , int const*        const last  = nullptr
    ) {
        auto& lvl = current_level();

        auto const subject_p = require_entity_on_level(subject, lvl);
        if (subject_p != to_p) {
            BK_ASSERT(false); //TODO
            return 0;
        }

        static_string_buffer<128> buffer;

        auto const lvl_loc = level_location {lvl, to_p};
        return move_items(subject, subject_p, first, last, subject, lvl_loc
            , always_true {}
            , [&](const_item_descriptor const i) {
                buffer.clear();
                buffer.append("%s drop the %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data());
                message_window.println(buffer.to_string());
            }
            , [&](const_item_descriptor const i, string_view const reason) {
                buffer.clear();
                buffer.append("%s can't drop the %s: %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data()
                  , reason.data());
                message_window.println(buffer.to_string());
            });
    }

    template <typename To>
    int remove_items(
        entity_descriptor const subject
      , item_descriptor   const container
      , To                const to
      , int const*        const first = nullptr
      , int const*        const last  = nullptr
    ) {
        auto const subject_p = require_entity_on_level(subject, current_level());

        static_string_buffer<128> buffer;

        return move_items(subject, subject_p, first, last, container, to
            , always_true {}
            , [&](const_item_descriptor const i) {
                buffer.clear();
                buffer.append("%s remove the %s from the %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data()
                  , name_of_decorated(ctx, container).data());
                message_window.println(buffer.to_string());
            }
            , [&](const_item_descriptor const i, string_view const reason) {
                buffer.clear();
                buffer.append("%s can't remove the %s from the %s: %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data()
                  , name_of_decorated(ctx, container).data()
                  , reason.data());
                message_window.println(buffer.to_string());
            });
    }

    int insert_items(
        entity_descriptor const subject
      , point2i32         const p
      , item_descriptor   const container
      , int const*        const first = nullptr
      , int const*        const last  = nullptr
    ) {
        static_string_buffer<128> buffer;

        return move_items(subject, p, first, last, subject, container
            , always_true {}
            , [&](const_item_descriptor const i) {
                buffer.clear();
                buffer.append("%s put the %s in the %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data()
                  , name_of_decorated(ctx, container).data());
                message_window.println(buffer.to_string());
            }
            , [&](const_item_descriptor const i, string_view const reason) {
                buffer.clear();
                buffer.append("%s can't put the %s in the %s: %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data()
                  , name_of_decorated(ctx, container).data()
                  , reason.data());
                message_window.println(buffer.to_string());
            });
    }

    void impl_do_drop_items_(int const n) {
        BK_ASSERT(n > 0);

        auto const player_p = player_location();
        auto const player_d = entity_descriptor {ctx, player_id()};

        if (n == 1 && player_d.obj.items().empty()) {
            message_window.println("You have nothing to drop.");
            return;
        }

        // called later; be careful of dangling references
        auto const handler = [=](command_type const cmd) {
            switch (cmd) {
            case command_type::alt_drop_some:
                BK_ATTRIBUTE_FALLTHROUGH;
            case command_type::confirm:
                drop_selected_items(player_d, player_p);
                break;
            case command_type::cancel:
                message_window.println("Nevermind.");
                renderer.update_tool_tip_visible(false);
                break;
            default:
                return event_result::filter;
            }

            return event_result::filter_detach;
        };

        item_list.assign(player_d.obj.items());

        if (n > 1) {
            choose_multiple_items("Drop which item(s)?", handler);
        } else {
            choose_single_item("Drop which item?", handler);
        }
    }

    //! For n > 0, choose which items to get.
    //! For n < 0, get all items
    //! @pre n != 0
    void impl_do_get_items_(int const n) {
        BK_ASSERT(n != 0);

        auto const player_p = player_location();
        auto const player_d = entity_descriptor {ctx, player_id()};

        auto* const pile = current_level().item_at(player_p);
        if (!pile) {
            message_window.println("There is nothing here to get.");
            return;
        }

        // called later; be careful of dangling references
        auto const handler = [=](command_type const cmd) {
            switch (cmd) {
            case command_type::alt_get_items:
                BK_ATTRIBUTE_FALLTHROUGH;
            case command_type::confirm:
                pickup_selected_items(player_d, player_p);
                break;
            case command_type::cancel:
                message_window.println("Nevermind.");
                renderer.update_tool_tip_visible(false);
                break;
            default:
                return event_result::filter;
            }

            return event_result::filter_detach;
        };

        if (n > 1) {
            item_list.assign(*pile);
            choose_multiple_items("Get which item(s)?", handler);
            return;
        }

        // n < 0 ~> get all items
        if (pickup_items(player_d, player_p) > 0) {
            renderer_update_pile(player_p);
        }
    }

    template <typename UnaryF>
    void query_yes_no(UnaryF callback) {
        input_context c(__func__);

        c.on_command_handler = [=](command_type const cmd, uint64_t) {
            auto const done = (cmd == command_type::cancel)
                           || (cmd == command_type::confirm);

            if (done) {
                callback(cmd);
                return event_result::filter_detach;
            }

            return event_result::filter;
        };

        c.on_text_input_handler = [=](text_input_event const event) {
            if (event.text.size() != 1u) {
                return event_result::filter;
            }

            auto const ch = event.text[0];

            if (ch == 'y' || ch == 'Y') {
                callback(command_type::confirm);
                return event_result::filter_detach;
            }

            if (ch == 'n' || ch == 'N') {
                callback(command_type::cancel);
                return event_result::filter_detach;
            }

            return event_result::filter;
        };

        context_stack.push(std::move(c));
    }

    //! Inserts items from the player's inventory into container.
    //! The item list is populated with the items in the players inventory
    //! excluding the container itself if the player is holding it.
    //! @pre container must be an item that is a container.
    void insert_into_container(item_descriptor const container) {
        auto const player_p = player_location();
        auto const player   = entity_descriptor {ctx, player_id()};

        auto const fill_list = [=, cid = container.obj.instance()] {
            item_list.assign_if(player.obj.items()
              , [&](item_instance_id const id) { return id != cid; });
        };

        auto const handler = [=](command_type const cmd) {
            switch (cmd) {
            case command_type::alt_insert:
                BK_ATTRIBUTE_FALLTHROUGH;
            case command_type::confirm:
                if (insert_selected_items(player, player_p, container)) {
                    fill_list();
                }
                break;
            case command_type::cancel:
                message_window.println("Nevermind.");
                renderer.update_tool_tip_visible(false);
                break;
            default:
                return event_result::filter;
            }

            return event_result::filter_detach;
        };

        fill_list();
        choose_multiple_items("Insert which item(s)?", handler);
    }

    //! Opens the indicated item from the item list if it is a container,
    //! otherwise does nothing. The list is populated with the contents of the
    //! indicated container.
    bool insert_into_indicated_container() {
        return item_list.with_indicated([&](item_instance_id const id) {
            auto const i = item_descriptor {ctx, id};
            if (is_container(i)) {
                insert_into_container(i);
            }
        });
    }

    //! @pre container must actually be a container
    void view_container(item_descriptor const container) {
        if (!is_identified(container)) {
            // viewing a container updates the id status to level 1
            container.obj.add_or_update_property(
                {property(item_property::identified), 1});
        }

        auto const player_info = get_player();
        auto&      player      = player_info.first;
        auto const player_p    = player_info.second;
        auto const player_d    = entity_descriptor {database, player};

        auto const name = name_of_decorated(ctx, container);

        // Print a message
        {
            static_string_buffer<128> buffer;
            buffer.append("You open the %s.", name.data());
            message_window.println(buffer.to_string());
        }

        item_list.set_title(name);
        item_list.assign(container.obj.items());
        item_list.set_modal(true);
        item_list.set_multiselect(true);
        item_list.show();

        item_list.set_on_command([=](command_type const cmd) {
            bool update = false;

            switch (cmd) {
            case command_type::alt_get_items:
                update = !!remove_selected_items(player_d, container, player_d);
                break;
            case command_type::alt_drop_some:
                update = !!remove_selected_items(player_d, container, level_location {current_level(), player_p});
                update_render_pile_if(player_p, update);
                break;
            case command_type::alt_open:
                break;
            case command_type::alt_insert:
                insert_into_indicated_container();
                return event_result::filter_detach;
            case command_type::cancel:
                renderer.update_tool_tip_visible(false);
                return event_result::filter_detach;
            default:
                break;
            }

            if (update) {
                item_list.set_title(name_of_decorated(ctx, container));
                item_list.assign(container.obj.items());
            }

            return event_result::filter;
        });
    }

    bool view_indicated_container() {
        return item_list.with_indicated([&](item_instance_id const id) {
            auto const i = item_descriptor {ctx, id};
            if (is_container(i)) {
                view_container(i);
            }
        });
    }

    void adjust_view_to_player(point2i32 const p) noexcept {
        constexpr int tile_distance_x = 5;
        constexpr int tile_distance_y = 5;

        auto const& tmap = database.get_tile_map(tile_map_type::base);
        auto const tw = tmap.tile_width();
        auto const th = tmap.tile_height();

        auto const win_r = underlying_cast_unsafe<float>(
            os.render_get_client_rect());

        auto const q = current_view.world_to_window(
            underlying_cast_unsafe<float>(p) + vec2f {0.5f, 0.5f}, tw, th);

        auto const left   = q.x - win_r.x0;
        auto const top    = q.y - win_r.y0;
        auto const right  = win_r.x1 - q.x;
        auto const bottom = win_r.y1 - q.y;

        auto const limit = current_view.world_to_window(
            vec2i32 {tile_distance_x * tw, tile_distance_y * th});

        auto const dx = (left  < limit.x) ? value_cast(limit.x - left)
                      : (right < limit.x) ? value_cast(right - limit.x)
                      : 0.0f;

        auto const dy = (top    < limit.y) ? value_cast(limit.y - top)
                      : (bottom < limit.y) ? value_cast(bottom - limit.y)
                      : 0.0f;

        update_view_trans(current_view.x_off + dx, current_view.y_off + dy);
    }

    placement_result impl_player_move_by_(level& lvl, entity& player, point2i32 const p, vec2i32 const v) {
        auto const result = lvl.move_by(player.instance(), v);
        if (result != placement_result::ok) {
            return result;
        }

        adjust_view_to_player(p + v);

        renderer_update(player.definition(), p, p + v);
        advance(1);

        return result;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Item transfer
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    template <typename Predicate>
    int move_items(level_location const l, int const* const first, int const* const last, Predicate pred) {
        return l.lvl.move_items(l.p, first, last, pred).second;
    }

    template <typename Predicate>
    int move_items(entity_descriptor const e, int const* const first, int const* const last, Predicate pred) {
        return e.obj.items().remove_if2(
            first, last, item_list_index_to_id(), pred);
    }

    template <typename Predicate>
    int move_items(item_descriptor const i, int const* const first, int const* const last, Predicate pred) {
        return i.obj.items().remove_if2(
            first, last, item_list_index_to_id(), pred);
    }

    template <typename From, typename To, typename Predicate, typename Success, typename Fail>
    int move_items(
        const_entity_descriptor const subject
      , point2i32               const //subject_p
      , int const* const first
      , int const* const last
      , From      from
      , To        to
      , Predicate filter
      , Success   on_success
      , Fail      on_failure
    ) {
        BK_ASSERT((!!first == !!last)
               && !!subject
               && !!from
               && !!to);

        auto const on_pass = [&](unique_item&& itm, item_descriptor const i) {
            on_success(i);
            merge_into_pile(ctx, std::move(itm), i, to);
            return true;
        };

        return move_items(from, first, last, [&](unique_item&& itm) {
            auto const i = item_descriptor {ctx, itm.get()};

            auto const on_fail = [&](string_view const reason) {
                on_failure(i, reason);
            };

            return filter(i)
                && can_remove_item(ctx, i, from, on_fail)
                && can_add_item(ctx, i, to, on_fail)
                && on_pass(std::move(itm), i);
        });
    }

    void renderer_update_pile(point2i32 const p) {
        auto const pile = the_world.current_level().item_at(p);
        if (!pile) {
            renderer_remove_item(p);
            return;
        }

        auto const pile_id = get_pile_id();
        renderer_add(get_pile_id(*pile, pile_id), p);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Commands
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void do_view() {
        set_highlighted_tile(get_player().second);

        input_context c;

        c.on_mouse_button_handler =
            [&](mouse_event const event, kb_modifiers const kmods) {
                using mbc = mouse_event::button_change_t;

                auto const ok =
                    (event.button_change[0] == mbc::went_down)
                 && (kmods.none());

                if (ok) {
                    set_highlighted_tile(
                        window_to_world({event.x, event.y}));
                }

                return event_result::filter;
            };

        c.on_command_handler =
            [&](command_type const type, uint64_t) {
                using ct = command_type;

                switch (type) {
                case ct::reset_view : BK_ATTRIBUTE_FALLTHROUGH;
                case ct::reset_zoom :
                    return event_result::pass_through;
                case ct::cancel :
                    highlighted_tile = point2i32 {-1, -1};
                    renderer.clear_tile_highlight();
                    renderer.update_tool_tip_visible(false);
                    adjust_view_to_player(get_player().second);
                    return event_result::filter_detach;
                case ct::move_n  : update_highlighted_tile({ 0, -1}); break;
                case ct::move_ne : update_highlighted_tile({ 1, -1}); break;
                case ct::move_e  : update_highlighted_tile({ 1,  0}); break;
                case ct::move_se : update_highlighted_tile({ 1,  1}); break;
                case ct::move_s  : update_highlighted_tile({ 0,  1}); break;
                case ct::move_sw : update_highlighted_tile({-1,  1}); break;
                case ct::move_w  : update_highlighted_tile({-1,  0}); break;
                case ct::move_nw : update_highlighted_tile({-1, -1}); break;
                default:
                    break;
                }

                return event_result::filter;
            };

        context_stack.push(std::move(c));
    }

    void do_cancel() {
        if (item_list.is_visible()) {
            item_list.set_modal(false);
            item_list.hide();
        }
    }

    void do_toggle_inventory() {
        if (!item_list.toggle_visible()) {
            return;
        }

        do_view_inventory();
    }

    void do_view_inventory() {
        item_list.set_title("Inventory");
        item_list.assign(find(the_world, player_id()).items());
        item_list.set_modal(false);
        item_list.set_multiselect(true);
        item_list.show();

        item_list.set_on_command([&](command_type const cmd) {
            bool update = false;

            switch (cmd) {
            case command_type::alt_drop_some: {
                auto const d = entity_descriptor {ctx, player_id()};
                auto const p = require_entity_on_level(d, current_level());
                if (drop_selected_items(d, p)) {
                    item_list.assign(d.obj.items());
                }
                break;
            }
            case command_type::alt_open:
                view_indicated_container();
                return event_result::filter_detach;
            case command_type::alt_insert:
                insert_into_indicated_container();
                return event_result::filter_detach;
            case command_type::cancel:
                renderer.update_tool_tip_visible(false);
                return event_result::filter_detach;
            default:
                break;
            }

            if (update) {
                item_list.assign(find(the_world, player_id()).items());
            }

            return event_result::filter;
        });
    }

    int pickup_items(
        entity_descriptor const subject
      , point2i32         const from_p
      , int const*        const first = nullptr
      , int const*        const last  = nullptr
    ) {
        auto& lvl = current_level();

        auto const subject_p = [&] {
            auto const result =
                current_level().find_position(subject.obj.instance());
            BK_ASSERT(result.first);
            return result.second;
        }();

        if (from_p != subject_p) {
            BK_ASSERT(false); //TODO
            return 0;
        }

        static_string_buffer<128> buffer;

        auto const lvl_loc = level_location {lvl, from_p};
        return move_items(subject, subject_p, first, last, lvl_loc, subject
            , always_true {}
            , [&](const_item_descriptor const i) {
                buffer.clear();
                buffer.append("%s pick up the %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data());
                message_window.println(buffer.to_string());
            }
            , [&](const_item_descriptor const i, string_view const reason) {
                buffer.clear();
                buffer.append("%s can't pick up the %s: %s."
                  , name_of_decorated(ctx, subject).data()
                  , name_of_decorated(ctx, i).data()
                  , reason.data());
                message_window.println(buffer.to_string());
            });
    }

    template <typename T>
    T update_render_pile_if(point2i32 const p, T const value) {
        if (!!value) {
            renderer_update_pile(p);
        }

        return value;
    }

    //! The entity e will attempt to pickup the items currently selected in the
    //! item list at location p on the current level.
    int pickup_selected_items(entity_descriptor const e, point2i32 const p) {
        using It = int const* const;
        return item_list.with_selected_range([&](It first, It last) {
            return update_render_pile_if(p, pickup_items(e, p, first, last));
        });
    }

    //! The entity e will attempt to pickup the items currently selected in the
    //! item list at location p on the current level.
    int drop_selected_items(entity_descriptor const e, point2i32 const p) {
        using It = int const* const;
        return item_list.with_selected_range([&](It first, It last) {
            return update_render_pile_if(p, drop_items(e, p, first, last));
        });
    }

    //! The entity e will attempt to pickup the items currently selected in the
    //! item list at location p on the current level.
    template <typename To>
    int remove_selected_items(entity_descriptor const e, item_descriptor const i, To const to) {
        using It = int const* const;
        return item_list.with_selected_range([&](It first, It last) {
            return remove_items(e, i, to, first, last);
        });
    }

    //! The entity e will attempt to pickup the items currently selected in the
    //! item list at location p on the current level.
    int insert_selected_items(entity_descriptor const e, point2i32 const p, item_descriptor const container) {
        using It = int const* const;
        return item_list.with_selected_range([&](It first, It last) {
            return insert_items(e, p, container, first, last);
        });
    }

    //! Pickup 0..N items from a list at the player's current position.
    void do_get_items() {
        impl_do_get_items_(2);
    }

    //! Pickup all items at the player's current position.
    void do_get_all_items() {
        impl_do_get_items_(-1);
    }

    //! Drop zero or one items from the player's inventory at the player's
    //! current position.
    void do_drop_one() {
        impl_do_drop_items_(1);
    }

    //! Drop zero or more items from the player's inventory at the player's
    //! current position.
    void do_drop_some() {
        impl_do_drop_items_(2);
    }

    void do_open() {
        auto const  player_info = get_player();
        auto&       player      = player_info.first;
        auto const  player_p    = player_info.second;
        auto&       lvl         = the_world.current_level();

        auto const is_container = [&](item_instance_id const id) noexcept {
            return boken::is_container({ctx, id}) > 0;
        };

        // check for containers at the players position
        auto const find_containers = [=](item_pile const* const pile) noexcept {
            using it_t = decltype(begin(*pile));

            // find items which are containers
            auto const find_container = [=](it_t const first, it_t const last) noexcept {
                return std::find_if(first, last, is_container);
            };

            // empty pile
            if (!pile) {
                return std::make_tuple(0, it_t {}, it_t {}, it_t {});
            }

            auto const last        = end(*pile);
            auto const first_match = find_container(begin(*pile), last);

            // no matches
            if (first_match == last) {
                return std::make_tuple(0, it_t {}, it_t {}, it_t {});
            }

            auto const second_match =
                find_container(std::next(first_match), last);

            // one match
            if (second_match == last) {
                return std::make_tuple(1, first_match, first_match, last);
            }

            // at least two matches
            return std::make_tuple(2, first_match, second_match, last);
        };

        // choose between multiple container choices
        auto const choose_container = [=](item_pile const& pile, auto const result) {
            // there are at least two containers here; build a list and let the
            // player decide which to inspect
            BK_ASSERT(std::get<0>(result) == 2);

            auto const it_1st_match = std::get<1>(result);
            auto const it_2nd_match = std::get<2>(result);
            auto const last         = std::get<3>(result);

            auto& il = item_list;

            il.clear();
            il.append(*it_1st_match);
            il.append(*it_2nd_match);
            il.append_if(std::next(it_2nd_match), last, is_container);
            il.layout();

            auto const handler = [&](command_type const cmd) {
                switch (cmd) {
                case command_type::alt_open: BK_ATTRIBUTE_FALLTHROUGH;
                case command_type::confirm:
                    view_indicated_container();
                    break;
                case command_type::cancel:
                    message_window.println("Nevermind.");
                    renderer.update_tool_tip_visible(false);
                    break;
                default:
                    return event_result::filter;
                }

                return event_result::filter_detach;
            };

            choose_single_item("Open which container?", handler);
        };

        //
        // first check for containers on the ground at the player's position
        //

        auto const pile    = lvl.item_at(player_p);
        auto const result  = find_containers(pile);
        auto const matches = std::get<0>(result);

        if (matches == 1) {
            auto const id = *std::get<1>(result);
            view_container({ctx, id});
            return;
        }

        if (matches == 2) {
            choose_container(*pile, result);
            return;
        }

        //
        // failing that, check if the player is holding any containers
        //

        BK_ASSERT(matches == 0);
        message_window.println("There is nothing here to open.");

        auto const& items     = player.items();
        auto const  p_result  = find_containers(&items);
        auto const  p_matches = std::get<0>(p_result);

        if (p_matches <= 0) {
            return;
        }

        //
        // the player has at least one container; ask whether they want to open
        // one of those instead.
        //

        if (p_matches == 2) {
            message_window.println("Open a container in your inventory? y/n");
            query_yes_no([=, &items](command_type const cmd) {
                if (cmd == command_type::confirm) {
                    choose_container(items, p_result);
                }
            });
            return;
        }

        BK_ASSERT(p_matches == 1);
        auto const container_id = *std::get<1>(p_result);
        auto const container_d  = item_descriptor {ctx, container_id};

        static_string_buffer<128> buffer;
        buffer.append("Open the %s in your inventory? y/n"
            , name_of_decorated(ctx, container_d).data());
        message_window.println(buffer.to_string());

        query_yes_no([=](command_type const cmd) {
            if (cmd == command_type::confirm) {
                view_container(container_d);;
            }
        });
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

                message_window.println("Done.");
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

                return event_result::filter;
            };

        context_stack.push(std::move(c));
    }

    void do_kill(entity& e, point2i32 const p) {
        auto& lvl = the_world.current_level();

        BK_ASSERT(!e.is_alive()
               && lvl.is_entity_at(p));

        static_string_buffer<128> buffer;
        buffer.append("The %s dies.", name_of_decorated(ctx, {database, e}).data());
        message_window.println(buffer.to_string());

        get_entity_loot(e, rng_superficial, [&](unique_item&& itm) {
            add_object_at(std::move(itm), p);
        });

        lvl.remove_entity(e.instance());
        renderer_remove_entity(p);
    }

    void do_combat(point2i32 const att_pos, point2i32 const def_pos) {
        auto& lvl = the_world.current_level();

        auto* const att = lvl.entity_at(att_pos);
        auto* const def = lvl.entity_at(def_pos);

        BK_ASSERT(!!att && !!def);
        BK_ASSERT(att->is_alive() && def->is_alive());

        def->modify_health(-1);

        if (!def->is_alive()) {
            do_kill(*def, def_pos);
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
            set_current_level(next_id, false);
        }

        // the level has been changed at this point

        auto const p = (delta > 0)
          ? the_world.current_level().stair_up(0)
          : the_world.current_level().stair_down(0);

        add_object_near(std::move(player_ent), player_id, p, 5, rng_substantive);

        reset_view_to_player();
    }

    placement_result do_player_move_to(point2i32 const p) {
        auto&      lvl         = the_world.current_level();
        auto const player_info = get_player();
        auto&      player      = player_info.first;
        auto const p_cur       = player_info.second;
        auto const p_dst       = p;

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

    void do_player_run(vec2i32 const v) {
        BK_ASSERT(value_cast(abs(v.x)) <= 1
               && value_cast(abs(v.y)) <= 1
               && v != vec2i32 {});

        constexpr auto timer_name = djb2_hash_32c("run timer");

        // add an input context that automatically terminates the run on
        // player input
        input_context c {__func__};

        c.on_mouse_button_handler = [=](auto, auto) {
            timers.remove(timer_name);
            return event_result::filter_detach;
        };

        c.on_command_handler = [=](auto, auto) {
            timers.remove(timer_name);
            return event_result::filter_detach;
        };

        auto const context_id = context_stack.push(std::move(c));

        using namespace std::chrono;
        constexpr auto delay = duration_cast<nanoseconds>(seconds {1}) / 100;

        auto&      lvl         = the_world.current_level();
        auto const player_info = get_player();
        auto&      player      = player_info.first;
        auto       p           = player_info.second;

        timers.add(timer_name, timer::duration {0}
          , [=, &lvl, &player, count = 0]
            (timer::duration, timer::timer_data) mutable -> timer::duration {
                auto const result = impl_player_move_by_(lvl, player, p, v);

                // continue running
                if (result == placement_result::ok) {
                    ++count;
                    p += v;
                    return delay;
                }

                // hit something before even moving one tile
                if (result == placement_result::failed_obstacle && count == 0) {
                    auto const q = p + v;
                    if (lvl.at(q).type == tile_type::door) {
                        interact_door(player, p, q);
                    }
                }

                context_stack.pop(context_id);
                return timer::duration {};
          });
    }

    placement_result do_player_move_by(vec2i32 const v) {
        BK_ASSERT(value_cast(abs(v.x)) <= 1
               && value_cast(abs(v.y)) <= 1
               && v != vec2i32 {});

        auto&      lvl         = the_world.current_level();
        auto const player_info = get_player();
        auto&      player      = player_info.first;
        auto const p_cur       = player_info.second;
        auto const p_dst       = p_cur + v;

        auto const result = impl_player_move_by_(lvl, player, p_cur, v);

        switch (result) {
        case placement_result::ok:
            break;
        case placement_result::failed_entity:
            do_combat(p_cur, p_dst);
            break;
        case placement_result::failed_obstacle:
            interact_obstacle(player, p_cur, p_dst);
            break;
        case placement_result::failed_bounds:
            break;
        case placement_result::failed_bad_id:
            // the player id should always be valid
            BK_ATTRIBUTE_FALLTHROUGH;
        default :
            BK_ASSERT(false);
            break;
        }

        return result;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Object creation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    unique_entity create_object(entity_definition const& def, random_state& rng) {
        return boken::create_object(the_world, def, rng);
    }

    unique_item create_object(item_definition const& def, random_state& rng) {
        return boken::create_object(the_world, def, rng);
    }

    template <typename Definition>
    auto impl_create_object_from_def_at_(Definition const& def, point2i32 const p, random_state& rng) {
        renderer_add(def.id, p);

        return the_world.current_level()
            .add_object_at(create_object(def, rng), p);
    }

    item_instance_id create_object_at(item_definition const& def, point2i32 const p, random_state& rng) {
        return impl_create_object_from_def_at_(def, p, rng);
    }

    entity_instance_id create_object_at(entity_definition const& def, point2i32 const p, random_state& rng) {
        return impl_create_object_from_def_at_(def, p, rng);
    }

    template <typename Definition>
    auto impl_create_object_from_id_at_(Definition const id, point2i32 const p, random_state& rng) {
        auto* const def = database.find(id);
        bool  const ok  = !!def;

        using instance_t = decltype(create_object_at(*def, p, rng));

        auto const instance_id = ok
          ? create_object_at(*def, p, rng)
          : instance_t {};

        return std::make_pair(instance_id, ok);
    }

    std::pair<item_instance_id, bool>
    create_object_at(item_id const id, point2i32 const p, random_state& rng) {
        return impl_create_object_from_id_at_(id, p, rng);
    }

    std::pair<entity_instance_id, bool>
    create_object_at(entity_id const id, point2i32 const p, random_state& rng) {
        return impl_create_object_from_id_at_(id, p, rng);
    }

    void create_item_in(item_instance_id const dest, item_definition const& def, random_state& rng) {
        auto itm = create_object(def, rng);
        boken::find(the_world, dest).add_item(std::move(itm));
    }

    void create_item_in(item& dest, item_definition const& def, random_state& rng) {
        create_item_in(dest.instance(), def, rng);
    }

    point2i32 add_object_near(
        unique_entity&& e
      , entity_id const id
      , point2i32 const p
      , int32_t   const distance
      , random_state&   rng
    ) {
        auto& lvl = the_world.current_level();

        auto const result =
            lvl.find_valid_entity_placement_neareast(rng, p, distance);

        BK_ASSERT(result.second == placement_result::ok);

        auto const q = result.first;

        lvl.add_object_at(std::move(e), q);
        renderer_add(id, q);

        return q;
    }

    item_instance_id add_object_at(unique_item&& i, point2i32 const p) {
        BK_ASSERT(!!i);
        auto const id = boken::find(the_world, i.get()).definition();
        renderer_add(id, p);
        return the_world.current_level().add_object_at(std::move(i), p);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void interact_door(entity& e, point2i32 const cur_pos, point2i32 const obstacle_pos) {
        auto& lvl = the_world.current_level();
        auto const tile = lvl.at(obstacle_pos);

        BK_ASSERT(tile.type == tile_type::door);

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

        update_tile_at(obstacle_pos, data);
    }

    void interact_obstacle(entity& e, point2i32 const cur_pos, point2i32 const obstacle_pos) {
        auto& lvl = the_world.current_level();

        auto const tile = lvl.at(obstacle_pos);
        if (tile.type == tile_type::door) {
            interact_door(e, cur_pos, obstacle_pos);
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

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Rendering
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

    //! Render the game
    void render(timepoint_t const last_frame) {
        using namespace std::chrono;

        constexpr auto frame_time =
            duration_cast<nanoseconds>(seconds {1}) / 60;

        auto const now   = clock_t::now();
        auto const delta = now - last_frame;

        if (delta < frame_time) {
            return;
        }

        if (!entity_updates_.empty()) {
            auto const n = static_cast<ptrdiff_t>(entity_updates_.size());
            auto const p = entity_updates_.data();
            renderer.update_data(p, p + n);
            entity_updates_.clear();
        }

        if (!item_updates_.empty()) {
            auto const n = static_cast<ptrdiff_t>(item_updates_.size());
            auto const p = item_updates_.data();
            renderer.update_data(p, p + n);
            item_updates_.clear();
        }

        renderer.render(delta, current_view);

        last_frame_time = now;
    }

    //! The main game loop
    void run() {
        while (os.is_running()) {
            timers.update();
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
        up<message_log>        messsage_window_ptr = make_message_log(*trender_ptr);
    } state {};

    system&             os              = *state.system_ptr;
    random_state&       rng_substantive = *state.rng_substantive_ptr;
    random_state&       rng_superficial = *state.rng_superficial_ptr;
    game_database&      database        = *state.database_ptr;
    world&              the_world       = *state.world_ptr;
    game_renderer&      renderer        = *state.renderer_ptr;
    text_renderer&      trender         = *state.trender_ptr;
    command_translator& cmd_translator  = *state.cmd_translator_ptr;
    message_log&        message_window  = *state.messsage_window_ptr;
    context const       ctx             = context {the_world, database};

    timer timers;

    item_list_controller item_list;

    input_context_stack context_stack;

    view current_view;

    int last_mouse_x = 0;
    int last_mouse_y = 0;

    point2i32 highlighted_tile {-1, -1};

    std::vector<game_renderer::update_t<item_id>>   item_updates_;
    std::vector<game_renderer::update_t<entity_id>> entity_updates_;

    timepoint_t last_frame_time {};
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

    boken::game_state game;
    game.run();

    return 0;
} catch (...) {
    return 1;
}
