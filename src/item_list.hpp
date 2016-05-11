#pragma once

#include "inventory.hpp"
#include "math_types.hpp"
#include "types.hpp"
#include "algorithm.hpp"

#include "bkassert/assert.hpp"

#include <memory>
#include <functional>
#include <string>
#include <initializer_list>

#include <cstdint>
#include <cstddef>

namespace boken { class game_database; }
namespace boken { class item_pile; }
namespace boken { struct kb_event; }
namespace boken { struct text_input_event; }
namespace boken { struct mouse_event; }
namespace boken { enum class command_type : uint32_t; }
namespace boken { enum class event_result : uint32_t; }

namespace boken {
    namespace detail { struct tag_kb_modifiers; }
    template <typename T> class flag_set;
    using kb_modifiers = flag_set<detail::tag_kb_modifiers>;
}

namespace boken {

class item_list_controller {
public:
    using on_command_t = std::function<event_result (command_type type)>;
    using on_focus_change_t = std::function<void (bool)>;
    using on_selection_change_t = std::function<void (int)>;
    using get_f = inventory_list::get_f;

    //--------------------------------------------------------------------------
    explicit item_list_controller(std::unique_ptr<inventory_list> list);

    void add_column(std::string heading, get_f getter);

    //--------------------------------------------------------------------------

    //! Set the on command handler.
    //! @note If called during an invocation of the current handler, the new
    //!       handler is queued until control returns from the handler. If a
    //!       new handler has been queued, the existing handler must detach
    //!       on completion.
    void set_on_command(on_command_t handler);

    //! reset the on_command handler to the default.
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

    //! Clears the list of any existing rows, adds new rows for each item in
    //! items, and adjusts the layout to fit said items.
    int assign(item_pile const& items);

    //! Clears the list of any existing rows, adds new rows for each item in
    //! items matching the predicate, and adjusts the layout to fit said items.
    template <typename Predicate>
    int assign_if(item_pile const& items, Predicate pred) {
        auto& il = get();

        clear();

        for_each_matching(items, pred, [&](item_instance_id const id) {
            il.add_row(id);
        });

        il.layout();

        return static_cast<int>(il.rows());
    }

    int assign_if(item_pile const& items, item_instance_id const id) {
        return assign_if(items, [id](item_instance_id const other_id) noexcept {
            return other_id == id;
        });
    }

    int assign_if_not(item_pile const& items, item_instance_id const id) {
        return assign_if(items, [id](item_instance_id const other_id) noexcept {
            return other_id != id;
        });
    }

    void append(std::initializer_list<item_instance_id> list);
    void append(item_instance_id id);

    template <typename FwdIt, typename Predicate>
    void append_if(FwdIt const first, FwdIt const last, Predicate pred) {
        for_each_matching(first, last, pred, [&](item_instance_id const id) {
            append(id);
        });
    }

    void remove_rows(int const* first, int const* last);

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
    //--------------------------------------------------------------------------

    // @param pred f(item_instance_id) -> {bool, item*, item_definition const*}
    template <typename Predicate, typename Unaryf>
    bool with_index_if(int const i, Predicate pred, Unaryf f) {
        auto const& il = get();
        BK_ASSERT((!il.empty())
               && (i < static_cast<int>(il.rows())));

        auto const id     = il.row_data(i);
        auto const result = pred(id);

        if (!std::get<0>(result)) {
            return false;
        }

        auto const itm = std::get<1>(result);
        auto const def = std::get<2>(result);

        BK_ASSERT(!!itm && !!def);

        f(*itm, *def);
        return true;
    }

    template <typename Unaryf>
    bool with_index(int const i, Unaryf f) {
        auto const& il = get();
        BK_ASSERT((!il.empty())
               && (i < static_cast<int>(il.rows())));

        f(il.row_data(i));
        return true;
    }

    template <typename Predicate, typename Unaryf>
    bool with_indicated_if(Predicate pred, Unaryf f) {
        auto const& il = get();
        return !il.empty() && with_index_if(il.indicated(), pred, f);
    }

    template <typename Unaryf>
    bool with_indicated(Unaryf f) {
        auto const& il = get();
        return !il.empty() && with_index(il.indicated(), f);
    }

    template <typename BinaryF>
    int with_selected_range(BinaryF f) {
        auto const& il = get();
        if (il.empty()) {
            return 0;
        }

        auto const sel = il.get_selection();
        if (sel.first == sel.second) {
            auto const i = il.indicated();
            return f(std::addressof(i), std::addressof(i) + 1);
        }

        return f(sel.first, sel.second);
    }

    template <typename Predicate, typename BinaryF>
    int for_each_selected_if(Predicate pred, BinaryF f) {
        return with_selected_range([&](int const* const first, int const* const last) {
            return std::accumulate(first, last, 0
              , [&](int const sum, int const i) {
                    return sum + with_index_if(i, pred, f);
                });
            });
    }

    template <typename BinaryF>
    int for_each_selected(BinaryF f) {
        return with_selected_if(always_true {}, f);
    }
private:
    void set_visible_(bool state) noexcept;

    void resize_(point2i32 p, vec2i32 v);

    event_result do_on_command_(command_type type);

    void do_on_selection_change_(int prev_sel);
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
