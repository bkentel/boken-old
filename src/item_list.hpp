#pragma once

#include "inventory.hpp"
#include "math_types.hpp"
#include "types.hpp"

#include <memory>
#include <functional>
#include <string>

#include <cstdint>
#include <cstddef>

namespace boken { class game_database; }
namespace boken { class item_pile; }
namespace boken { struct kb_event; }
namespace boken { struct kb_modifiers; }
namespace boken { struct text_input_event; }
namespace boken { struct mouse_event; }
namespace boken { enum class command_type : uint32_t; }

namespace boken {

enum class event_result : uint32_t {
    filter              //!< filter the event
  , filter_detach       //!< detach and filter the event
  , pass_through        //!< pass through to the next handler
  , pass_through_detach //!< detach and pass through to the next handler
};

class item_list_controller {
public:
    using on_command_t = std::function<event_result (command_type type, int const* first, int const* last)>;
    using on_focus_change_t = std::function<void (bool)>;
    using on_selection_change_t = std::function<void (item_instance_id, point2i32)>;

    //--------------------------------------------------------------------------
    explicit item_list_controller(std::unique_ptr<inventory_list> list);

    void add_column(std::string heading, std::function<std::string (item const&)> getter);

    //--------------------------------------------------------------------------
    void set_on_command(on_command_t handler);
    void set_on_command();

    void set_on_focus_change(on_focus_change_t handler);
    void set_on_focus_change();

    void set_on_selection_change(on_selection_change_t handler);
    void set_on_selection_change();

    //--------------------------------------------------------------------------
    bool on_key(kb_event const& event, kb_modifiers const& kmods);

    bool on_text_input(text_input_event const& event);

    bool on_mouse_button(mouse_event const& event, kb_modifiers const& kmods);

    bool on_mouse_move(mouse_event const& event, kb_modifiers const& kmods);

    bool on_mouse_wheel(int wy, int wx, kb_modifiers const& kmods);

    bool on_command(command_type type, uint64_t const data);

    //--------------------------------------------------------------------------

    //! clears all state and row data; leaves columns intact.
    void clear();

    void assign(item_pile const& items);

    void append(item_instance_id const id);

    void layout();

    void set_title(std::string title);

    //--------------------------------------------------------------------------
    bool set_modal(bool state) noexcept;

    bool is_modal() const noexcept;

    bool set_multiselect(bool state) noexcept;

    bool is_multiselect() const noexcept;

    bool has_focus() const noexcept;
    //--------------------------------------------------------------------------
    void show() noexcept;

    void hide() noexcept;

    //! @returns the visible state of the list after toggling.
    bool toggle_visible() noexcept;

    bool is_visible() const noexcept;

    //--------------------------------------------------------------------------
    inventory_list const& get() const noexcept { return *list_; }
    inventory_list&       get()       noexcept { return *list_; }
private:
    void set_visible_(bool state) noexcept;

    void resize_(point2i32 p, vec2i32 v);

    event_result do_on_command_(command_type type, int const* first, int const* last);
private:
    std::unique_ptr<inventory_list> list_;

    on_command_t          on_command_;
    on_command_t          on_command_swap_;
    on_focus_change_t     on_focus_change_;
    on_selection_change_t on_selection_change_;

    point2i32    last_mouse_  {};
    inventory_list::hit_test_result last_hit_ {};

    bool is_moving_       {false};
    bool is_sizing_       {false};
    bool is_modal_        {false};
    bool is_multi_select_ {true};
    bool is_processing_callback_ {false};
};

} // namespace boken
