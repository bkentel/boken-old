#pragma once

#include "inventory.hpp"
#include "math_types.hpp"
#include "types.hpp"
#include "algorithm.hpp"
#include "functional.hpp"
#include "context_fwd.hpp"

#include <algorithm>
#include <numeric>
#include <memory>
#include <functional>
#include <string>
#include <initializer_list>
#include <type_traits>

#include <cstdint>
#include <cstddef>

//=====--------------------------------------------------------------------=====
//                            Forward Declarations
//=====--------------------------------------------------------------------=====
namespace boken {

class game_database;
class item_pile;
struct kb_event;
struct text_input_event;
struct mouse_event;
enum class command_type : uint32_t;
enum class event_result : uint32_t;

namespace detail { struct tag_kb_modifiers; }
template <typename> class flag_set;
using kb_modifiers = flag_set<detail::tag_kb_modifiers>;

} // namespace boken
//=====--------------------------------------------------------------------=====

namespace boken {

class item_list_controller {
public:
    using on_command_t          = std::function<event_result (command_type)>;
    using on_focus_change_t     = std::function<void (bool)>;
    using on_selection_change_t = std::function<void (int)>;
    using get_f                 = inventory_list::get_f;
    using sort_f                = inventory_list::sort_f;

    enum class column_type {
        icon, name, weight, count
    };

    enum class flag_type : uint32_t {
        visible     = 1u << 0
      , modal       = 1u << 1
      , multiselect = 1u << 2
    };

    //--------------------------------------------------------------------------
    virtual ~item_list_controller();

    //--------------------------------------------------------------------------
    virtual void set_flags(std::initializer_list<flag_type> flags) = 0;

    virtual void set_properties(
        std::string title, std::initializer_list<flag_type> flags) = 0;

    //! use a custom sort
    virtual void add_column(std::string heading, get_f getter, sort_f sorter) = 0;

    //! use a string based sort
    virtual void add_column(std::string heading, get_f getter) = 0;

    //! add a standard column
    virtual void add_column(const_context ctx, column_type type) = 0;

    virtual void add_columns(
        const_context ctx
      , column_type const* first
      , column_type const* last) = 0;

    virtual void add_columns(
        const_context ctx, std::initializer_list<column_type> list) = 0;

    //--------------------------------------------------------------------------

    //! Set the on command handler.
    virtual void set_on_command(on_command_t handler) = 0;
    virtual void set_on_command() = 0;

    virtual void set_on_focus_change(on_focus_change_t handler) = 0;
    virtual void set_on_focus_change() = 0;

    virtual void set_on_selection_change(on_selection_change_t handler) = 0;
    virtual void set_on_selection_change() = 0;

    //--------------------------------------------------------------------------
    virtual bool on_key(kb_event event, kb_modifiers kmods) = 0;
    virtual bool on_text_input(text_input_event const& event) = 0;
    virtual bool on_mouse_button(mouse_event event, kb_modifiers kmods) = 0;
    virtual bool on_mouse_move(mouse_event event, kb_modifiers kmods) = 0;
    virtual bool on_mouse_wheel(int wy, int wx, kb_modifiers kmods) = 0;
    virtual bool on_command(command_type type, uint64_t data) = 0;

    //--------------------------------------------------------------------------

    //! clears all state and row data; leaves columns intact.
    virtual void clear() = 0;

    //! Clears the list of any existing rows, adds new rows for each item in
    //! items, and adjusts the layout to fit said items.
    virtual int assign(item_pile const& items) = 0;

    virtual void append(std::initializer_list<item_instance_id> list) = 0;
    virtual void append(item_instance_id id) = 0;

    virtual void remove_rows(int const* first, int const* last) = 0;

    virtual void layout() = 0;

    virtual void set_title(std::string title) = 0;

    virtual bool cancel_modal() noexcept = 0;
    virtual bool cancel_selection() noexcept = 0;
    virtual bool cancel_all() noexcept = 0;

    //--------------------------------------------------------------------------
    virtual bool has_selection() const noexcept = 0;

    virtual bool set_modal(bool state) noexcept = 0;
    virtual bool is_modal() const noexcept = 0;

    virtual bool set_multiselect(bool state) noexcept = 0;
    virtual bool is_multiselect() const noexcept = 0;

    virtual bool has_focus() const noexcept = 0;

    virtual bool is_visible() const noexcept = 0;
    virtual void set_visible(bool state) noexcept = 0;

    //! @returns the visible state of the list after toggling.
    virtual bool toggle_visible() noexcept = 0;
    virtual void show() noexcept = 0;
    virtual void hide() noexcept = 0;

    //--------------------------------------------------------------------------
    virtual inventory_list const& get() const noexcept = 0;
    virtual inventory_list&       get()       noexcept = 0;
    //--------------------------------------------------------------------------
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
};

std::unique_ptr<item_list_controller> make_item_list_controller(
    std::unique_ptr<inventory_list> list);

} // namespace boken
