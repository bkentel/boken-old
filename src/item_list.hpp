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

class item_list_controller {
public:
    using on_confirm_t = std::function<void (int const* first, int const* last)>;
    using on_cancel_t  = std::function<void ()>;

    //--------------------------------------------------------------------------
    explicit item_list_controller(std::unique_ptr<inventory_list> list);

    void add_column(std::string heading, std::function<std::string (item const&)> getter);

    //--------------------------------------------------------------------------
    void on_confirm(on_confirm_t handler);

    void on_cancel(on_cancel_t handler);

    void reset_callbacks();

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

    //--------------------------------------------------------------------------
    void show() noexcept;

    void hide() noexcept;

    //! @returns the visible state of the list after toggling.
    bool toogle_visible() noexcept;

    bool is_visible() const noexcept;

    //--------------------------------------------------------------------------
    inventory_list const& get() const noexcept { return *list_; }
    inventory_list&       get()       noexcept { return *list_; }
private:
    void set_visible_(bool state) noexcept;

    void resize_(point2i32 p, vec2i32 v);
private:
    std::unique_ptr<inventory_list> list_;

    on_confirm_t on_confirm_;
    on_cancel_t  on_cancel_;

    point2i32    last_mouse_  {};
    inventory_list::hit_test_result last_hit_ {};

    bool is_moving_       {false};
    bool is_sizing_       {false};
    bool is_modal_        {false};
    bool is_multi_select_ {true};
};

} // namespace boken
