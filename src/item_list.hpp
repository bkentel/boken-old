#pragma once

#include "inventory.hpp"
#include "math_types.hpp"
#include "types.hpp"
#include "algorithm.hpp"
#include "functional.hpp"
#include "context_fwd.hpp"

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
    using get_f  = inventory_list::get_f;
    using sort_f = inventory_list::sort_f;

    enum class column_type {
        icon, name, weight, count
    };

    enum class flag_type : uint32_t {
        visible     = 1u << 0
      , modal       = 1u << 1
      , multiselect = 1u << 2
    };

    void set_properties(std::string title, std::initializer_list<flag_type> flags);

    //--------------------------------------------------------------------------
    explicit item_list_controller(std::unique_ptr<inventory_list> list);

    //! use a custom sort
    void add_column(std::string heading, get_f getter, sort_f sorter);

    //! use a string based sort
    void add_column(std::string heading, get_f getter);

    //! add a standard column
    void add_column(const_context ctx, column_type type);

    void add_columns(const_context ctx
        , column_type const* first, column_type const* last);

    void add_columns(const_context ctx, std::initializer_list<column_type> list);

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

    template <typename FwdIt, typename Predicate, typename Transform>
    void append_if(FwdIt const first, FwdIt const last, Predicate pred, Transform trans) {
        using value_t = std::decay_t<decltype(*first)>;
        for_each_matching(first, last, pred, [&](value_t const& value) {
            append(trans(value));
        });
    }

    void remove_rows(int const* first, int const* last);

    void layout();

    void set_title(std::string title);

    //--------------------------------------------------------------------------
    bool has_selection() const noexcept;

    bool set_modal(bool state) noexcept;

    bool is_modal() const noexcept;

    bool set_multiselect(bool state) noexcept;

    bool is_multiselect() const noexcept;

    bool has_focus() const noexcept;

    void set_visible(bool state) noexcept;
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
    template <typename Unaryf>
    bool with_indicated(Unaryf f) {
        auto const& il = get();
        return !il.empty() && ((void)f(il.row_data(il.indicated())), true);
    }

    //! If a selection exists, calls f with a pointer to the first and last
    //! element in the selection.
    //! If the list is empty, f is not called.
    //! Otherwise, calls f with a range of one element consisting of the
    //! currently indicated item
    //!
    //! @tparam BinaryF int (int const* beg, int const* end)
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

    template <typename UnaryF>
    int for_each_selected(UnaryF f) {
        auto const& il = get();
        auto const g = void_as_bool<true>(f);

        using It = int const*;
        return with_selected_range([&](It const first, It const last) {
            int n = 0;

            for (auto it = first; it != last; ++it) {
                if (g(il.row_data(*it))) {
                    ++n;
                }
            }

            return n;
        });
    }

    // for successive calls:
    // first, cancel modal state if set, and return false
    // then, remove any selection, and return false
    // finally return true
    bool cancel() noexcept;

    // Behaves as if calling cancel until it returns true.
    bool cancel_force() noexcept;
private:
    void resize_(point2i32 p, vec2i32 v);

    event_result do_on_command_(command_type type);

    void do_on_selection_change_(int prev_sel);
private:
    std::unique_ptr<inventory_list> list_;

    std::vector<on_command_t> command_stack_;
    on_command_t              on_command_;
    on_command_t              on_command_swap_;

    on_focus_change_t     on_focus_change_;
    on_selection_change_t on_selection_change_;

    point2i32 last_mouse_  {};
    inventory_list::hit_test_result last_hit_ {};

    // The current set of columns to sort by.
    std::vector<int> sort_cols_;

    bool is_moving_       {false};
    bool is_sizing_       {false};
    bool is_modal_        {false};
    bool is_multi_select_ {true};
    bool is_processing_callback_ {false};
};

} // namespace boken
