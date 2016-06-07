#include "item_list.hpp"
#include "system_input.hpp"
#include "scope_guard.hpp"
#include "command.hpp"
#include "item_pile.hpp"
#include "events.hpp"
#include "names.hpp"
#include "item_properties.hpp"

#include "bkassert/assert.hpp"

#include <algorithm>
#include <numeric>

namespace boken {

item_list_controller::~item_list_controller() = default;

class item_list_controller_impl final : public item_list_controller {
public:
    void set_flags(
        std::initializer_list<flag_type> const flags
    ) final override {
        set_flags_(std::accumulate(begin(flags), end(flags), uint32_t {0}
          , [](uint32_t const sum, flag_type const f) noexcept {
                return sum | static_cast<uint32_t>(f);
            }));
    }

    void set_properties(
        std::string title
      , std::initializer_list<flag_type> const flags
    ) final override {
        set_title(std::move(title));
        set_flags(flags);
    }

    void add_column(std::string heading, get_f getter, sort_f sorter) final override {
        auto const id = static_cast<uint8_t>(list_->cols() & 0xFFu);
        list_->add_column(id, std::move(heading), std::move(getter), std::move(sorter));
    }

    void add_column(std::string heading, get_f getter) final override {
        auto const by_string = [](
            const_item_descriptor, string_view const lhs
          , const_item_descriptor, string_view const rhs
        ) noexcept {
            return lhs.compare(rhs);
        };

        add_column(std::move(heading), std::move(getter), by_string);
    }

    void add_column(
        const_context const ctx
      , column_type   const type
    ) final override {
        using id_t = const_item_descriptor;

        switch (type) {
        case column_type::icon:
            add_column(" ", [=](id_t const i) {
                return name_of(ctx, i).substr(0, 1).to_string();
            });
            break;
        case column_type::name:
            add_column("Name", [=](id_t const i) {
                return name_of_decorated(ctx, i);
            });
            break;
        case column_type::weight:
            add_column("Weight"
              , [=](id_t const i) {
                    return std::to_string(weight_of_inclusive(ctx, i));
                }
              , [=](id_t const lhs, string_view, id_t const rhs, string_view) {
                    return compare(weight_of_inclusive(ctx, lhs)
                                 , weight_of_inclusive(ctx, rhs));
                });
            break;
        case column_type::count:
            add_column("Count"
              , [=](id_t const i) {
                    return std::to_string(current_stack_size(i));
                }
              , [=](id_t const lhs, string_view, id_t const rhs, string_view) {
                    return compare(current_stack_size(lhs)
                                 , current_stack_size(rhs));
                });
            break;
        default:
            BK_ASSERT(false);
            break;
        }
    }

    void add_columns(
        const_context      const ctx
      , column_type const* const first
      , column_type const* const last
    ) final override {
        for (auto it = first; it != last; ++it) {
            add_column(ctx, *it);
        }
    }

    void add_columns(
        const_context const ctx
      , std::initializer_list<column_type> const list
    ) final override {
        add_columns(ctx, list.begin(), list.end());
    }

    //--------------------------------------------------------------------------
    void set_on_command(on_command_t handler) final override {
        if (!is_processing_callback_) {
            BK_ASSERT(on_command_);
            command_stack_.push_back(get_state_(std::move(on_command_)));
            on_command_ = std::move(handler);
            on_command_(command_type::none);
        } else {
            BK_ASSERT(!on_command_swap_.on_command);
            on_command_swap_ = get_state_(std::move(handler));
        }
    }

    void set_on_command() final override {
        BK_ASSERT(!is_processing_callback_);
        on_command_ = [](auto) noexcept { return event_result::pass_through; };
    }

    void set_on_focus_change(on_focus_change_t handler) final override {
        on_focus_change_ = std::move(handler);
    }

    void set_on_focus_change() final override {
        on_focus_change_ = [](auto) noexcept {};
    }

    void set_on_selection_change(on_selection_change_t handler) final override {
        on_selection_change_ = std::move(handler);
    }

    void set_on_selection_change() final override {
        on_selection_change_ = [](auto) noexcept {};
    }

    //--------------------------------------------------------------------------

    bool on_key(kb_event const event, kb_modifiers const kmods) final override {
        if (!is_visible()) {
            return true;
        }

        return true;
    }

    bool on_text_input(text_input_event const& event) final override {
        if (!is_visible()) {
            return true;
        }

        BK_ASSERT(event.text.size() >= 1);

        if (event.text.size() > 1) {
            return true;
        }

        int const c = event.text[0];
        // a-z => 0  - 25
        // A-Z => 26 - 51
        // 0-9 => 52 - 61
        if (c >= 'a' && c <= 'z') {
            return false;
        } else if (c >= 'A' && c <= 'Z') {
            return false;
        } else if (c >= '0' && c <= '9') {
            return false;
        }

        return true;
    }

    bool on_mouse_button(mouse_event const event, kb_modifiers const kmods) final override {
        using type  = inventory_list::hit_test_result::type;
        using btype = mouse_event::button_change_t;

        auto& il = *list_;

        if (!is_visible()) {
            return !is_modal();
        }

        // reset and size / move state
        is_moving_ = false;
        is_sizing_ = false;

        // next, do a hit test, and passthrough if it fails. Otherwise, filter.
        auto const hit_test = il.hit_test({event.x, event.y});
        if (!hit_test) {
            return !is_modal();
        }

        // only interested in the first (left) mouse button
        if (event.button_change[0] == btype::none) {
            return false;
        }

        auto on_exit = BK_SCOPE_EXIT {
            last_hit_ = hit_test;
        };

        if (event.button_change[0] == btype::went_down) {
            if (hit_test.what == type::title) {
                is_moving_ = true;
            } else if (hit_test.what == type::frame) {
                is_sizing_   = true;
            }
        } else if (event.button_change[0] == btype::went_up) {
            if (hit_test.what == type::cell) {
                if (!is_multi_select_) {
                    do_on_command_(command_type::confirm);
                    return !is_visible();
                }

                if (kmods.exclusive_any(kb_mod::shift)) {
                    il.selection_toggle(hit_test.y);
                } else {
                    il.selection_set({hit_test.y});
                }
            } else if (hit_test.what == type::header) {
                auto const value = hit_test.x + 1;
                auto const last = end(sort_cols_);
                auto const it = std::lower_bound(begin(sort_cols_), last, value
                  , [](int const lhs, int const rhs) noexcept {
                        return std::abs(lhs) < std::abs(rhs);
                    });

                auto const is_match = [&] {
                    return it != last && std::abs(*it) == value;
                };

                if (!kmods.exclusive_any(kb_mod::shift)) {
                    sort_cols_.assign({is_match() ? -*it : value});
                } else {
                    if (is_match()) {
                        *it = -*it;
                    } else {
                        sort_cols_.insert(it, value);
                    }
                }

                il.sort(sort_cols_.data(), sort_cols_.data() + sort_cols_.size());
                il.layout();

                return false;
            }
        }

        return false;
    }

    bool on_mouse_move(mouse_event const event, kb_modifiers const kmods) final override {
        using type  = inventory_list::hit_test_result::type;

        auto const p = point2i32 {event.x, event.y};
        auto const v = p - last_mouse_;

        auto on_exit_mouse = BK_SCOPE_EXIT {
            last_mouse_ = p;
        };

        auto& il = *list_;

        if (!is_visible()) {
            return !is_modal();
        }

        // first, take care of any moving or sizing
        if (is_moving_) {
            il.move_by(v);
            return false;
        } else if (is_sizing_) {
            resize_(p, v);
            return false;
        }

        // next, do a hit test, and passthrough if it fails. Otherwise, filter.
        auto const hit_test = il.hit_test(p);

        auto const modal = is_modal();
        if (!modal) {
            auto const hit_test_before = il.hit_test(last_mouse_);

            // check if the mouse entered / exited and notify
            if (!hit_test_before && hit_test) {
                on_focus_change_(true);
            } else if (hit_test_before && !hit_test) {
                on_focus_change_(false);
            }
        }

        if (!hit_test) {
            return !modal;
        }

        // indicate the row the mouse is over
        if (hit_test.what == type::cell && event.button_state_bits() == 0) {
            do_on_selection_change_(il.indicate(hit_test.y));
        }

        return false;
    }

    bool on_mouse_wheel(int const wy, int const wx, kb_modifiers const kmods) final override {
        auto& il = *list_;

        if (!is_visible()) {
            return true;
        }

        if (!is_modal() && !il.hit_test(last_mouse_)) {
            return true;
        }

        if (wy > 0) {
            do_on_selection_change_(il.indicate_prev(wy));
            return false;
        } else if (wy < 0) {
            do_on_selection_change_(il.indicate_next(-wy));
            return false;
        }

        return true;
    }

    bool on_command(command_type const type, uint64_t const data) final override;

    //--------------------------------------------------------------------------

    void clear() final override {
        auto& il = *list_;

        il.clear_rows();
        il.selection_clear();
    }

    int assign(item_pile const& items) final override {
        clear();

        auto& il = *list_;

        il.reserve(il.cols(), items.size());

        std::for_each(begin(items), end(items), [&](item_instance_id const id) {
            il.add_row(id);
        });

        il.layout();

        return static_cast<int>(il.rows());
    }

    void append(std::initializer_list<item_instance_id> const list) final override {
        list_->add_rows(begin(list), end(list));
    }

    void append(item_instance_id const id) final override {
        list_->add_row(id);
    }

    void remove_rows(
        int const* const first
      , int const* const last
    ) final override {
        list_->remove_rows(first, last);
    }

    void layout() final override {
        list_->layout();
    }

    void set_title(std::string title) final override {
        list_->set_title(std::move(title));
    }

    bool cancel_modal() noexcept final override {
        bool const result = is_modal();
        return (result && set_modal(false)), result;
    }

    bool cancel_selection() noexcept final override {
        bool const result = has_selection();
        return (result && list_->selection_clear()), result;
    }

    bool cancel_all() noexcept final override {
        BK_ASSERT(!is_processing_callback_
               && !on_command_swap_.on_command);

        // no short circuit here -- we want both to be called regardless.
        bool const result =
            cancel_modal() | cancel_selection();

        if (command_stack_.empty()) {
            return result;
        }

        while (command_stack_.size() > 1) {
            command_stack_.pop_back();
        }

        on_command_ = std::move(command_stack_.back().on_command);
        command_stack_.pop_back();
        hide();

        return true;
    }

    //--------------------------------------------------------------------------
    bool has_selection() const noexcept final override {
        //TODO: a bit wasteful
        return !!list_->get_selection().first;
    }

    bool set_modal(bool const state) noexcept final override {
        bool const result = is_modal_;
        is_modal_ = state;

        if (result && !is_modal_ && !list_->hit_test(last_mouse_)) {
            // became non-modal
            on_focus_change_(false);
        } else if (!result && is_modal_ && !list_->hit_test(last_mouse_)) {
            // became modal
            on_focus_change_(true);
        }

        return result;
    }

    bool is_modal() const noexcept final override {
        return is_modal_;
    }

    bool set_multiselect(bool const state) noexcept final override {
        bool const result = is_multi_select_;
        is_multi_select_ = state;
        return result;
    }

    bool is_multiselect() const noexcept final override {
        return is_multi_select_;
    }

    bool has_focus() const noexcept final override {
        return is_visible() && (is_modal() || list_->hit_test(last_mouse_));
    }

    bool is_visible() const noexcept final override {
        return list_->is_visible();
    }

    void set_visible(bool const state) noexcept final override {
        auto const prev_state = is_visible();

        is_moving_ = false;
        is_sizing_ = false;
        state ? list_->show() : list_->hide();

        if (!prev_state && state && (is_modal() || list_->hit_test(last_mouse_))) {
            // became visible
            on_focus_change_(true);
        } else if (prev_state && !state) {
            // became invisible
            on_focus_change_(false);
        }
    }

    bool toggle_visible() noexcept final override {
        bool const is_visible = list_->is_visible();
        set_visible(!is_visible);
        return !is_visible;
    }

    void show() noexcept final override {
        set_visible(true);
    }

    void hide() noexcept final override {
        set_visible(false);
    }

    //--------------------------------------------------------------------------
    inventory_list const& get() const noexcept final override { return *list_; }
    inventory_list&       get()       noexcept final override { return *list_; }
public:
    explicit item_list_controller_impl(std::unique_ptr<inventory_list> list)
      : list_ {std::move(list)}
    {
        set_on_command();
        set_on_focus_change();
        set_on_selection_change();
    }
private:
    struct state_t {
        std::string  title;
        uint32_t     flags;
        int32_t      indicated;
        vec2i32      scroll;
        on_command_t on_command;
    };
private:
    void resize_(point2i32 const p, vec2i32 const v) {
        auto const crossed_x = [&](auto const x) noexcept {
            return ((last_mouse_.x <= x) && (p.x >= x))
                || ((last_mouse_.x >= x) && (p.x <= x));
        };

        auto const crossed_y = [&](auto const y) noexcept {
            return ((last_mouse_.y <= y) && (p.y >= y))
                || ((last_mouse_.y >= y) && (p.y <= y));
        };

        auto const frame = list_->metrics().frame;

        bool const ok_x =
            ((last_hit_.x < 0) && ((value_cast(v.x) > 0) || crossed_x(frame.x0)))
         || ((last_hit_.x > 0) && ((value_cast(v.x) < 0) || crossed_x(frame.x1)));

        bool const ok_y =
            ((last_hit_.y < 0) && ((value_cast(v.y) > 0) || crossed_y(frame.y0)))
         || ((last_hit_.y > 0) && ((value_cast(v.y) < 0) || crossed_y(frame.y1)));


        if (!ok_x && !ok_y) {
            return;
        }

        auto const dw = ok_x ? v.x - sizei32x {} : sizei32x {};
        auto const dh = ok_y ? v.y - sizei32y {} : sizei32y {};

        list_->resize_by(dw, dh, last_hit_.x, last_hit_.y);
    }

    event_result do_on_command_(command_type const type) {
        // the temporary should always be empty here
        BK_ASSERT(!is_processing_callback_ && !on_command_swap_.on_command);

        is_processing_callback_ = true;
        auto on_exit = BK_SCOPE_EXIT {
            is_processing_callback_ = false;
        };

        return on_command_(type);
    }

    void do_on_selection_change_(int const prev_sel) {
        auto& il = *list_;

        auto const i = il.indicated();
        if (i == prev_sel) {
            return;
        }

        il.scroll_into_view(0, i);
        on_selection_change_(i);
    }

    void set_flags_(uint32_t const flags) {
        using type = std::underlying_type_t<flag_type>;
        auto const has_flag = [=](flag_type const flag) noexcept {
            return !!(flags & static_cast<type>(flag));
        };

        set_visible(has_flag(flag_type::visible));
        set_modal(has_flag(flag_type::modal));
        set_multiselect(has_flag(flag_type::multiselect));
    }

    uint32_t get_flags_() const noexcept {
        return (is_visible()     ? static_cast<uint32_t>(flag_type::visible)     : 0u)
             | (is_modal()       ? static_cast<uint32_t>(flag_type::modal)       : 0u)
             | (is_multiselect() ? static_cast<uint32_t>(flag_type::multiselect) : 0u);
    }

    state_t get_state_(on_command_t&& handler) const {
        return {
            list_->title_text().to_string()
          , get_flags_()
          , list_->indicated()
          , list_->scroll_offset()
          , std::move(handler)
        };
    }
private:
    std::unique_ptr<inventory_list> list_;

    std::vector<state_t> command_stack_;
    on_command_t         on_command_;
    state_t              on_command_swap_;

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

std::unique_ptr<item_list_controller> make_item_list_controller(
    std::unique_ptr<inventory_list> list
) {
    return std::make_unique<item_list_controller_impl>(std::move(list));
}

bool item_list_controller_impl::on_command(
    command_type const type
  , uint64_t     const data
) {
    using ct = command_type;

    auto on_exit = BK_SCOPE_EXIT {
        // there should never be a queued handler when we finish
        BK_ASSERT(!on_command_swap_.on_command);
    };

    auto& il = *list_;

    // pass through the command if the list isn't visible, or if the list isn't
    // modal and the mouse isn't over the list
    if (!is_visible() || (!is_modal() && !il.hit_test(last_mouse_))) {
        return true;
    }

    // handle special commands
    auto const result = [&] {
        if (type == ct::move_n) {
            do_on_selection_change_(il.indicate_prev());
        } else if (type == ct::move_s) {
            do_on_selection_change_(il.indicate_next());
        } else if (type == ct::toggle) {
            if (is_multi_select_) {
                il.selection_toggle(il.indicated());
            }
        } else {
            return do_on_command_(type);
        }

        return event_result::filter;
    }();

    bool const detach = (result == event_result::filter_detach)
                     || (result == event_result::pass_through_detach);

    bool const filter = (result == event_result::filter)
                     || (result == event_result::filter_detach)
                     || is_modal();

    // A new handler was set during the on_command callback
    // swap it with the current one.
    if (on_command_swap_.on_command) {
        std::swap(on_command_swap_.on_command, on_command_);
        if (!detach) {
            command_stack_.push_back(std::move(on_command_swap_));
        }

        on_command_swap_.on_command = on_command_t {};
        do_on_command_(command_type::none);
    } else if (detach) {
        // The default handler should never not be on the stack if a user
        // handler is being detached
        BK_ASSERT(!command_stack_.empty());

        // Restore the previous handler
        auto& top = command_stack_.back();

        on_command_ = std::move(top.on_command);
        set_flags_(top.flags);
        set_title(std::move(top.title));

        // First let the handler repopulate the list
        do_on_command_(command_type::none);
        // Then set the indicated item
        list_->indicate(top.indicated);

        command_stack_.pop_back();

        // If there are no items left in the stack, we've hit the default handler
        // and should hide the list
        if (command_stack_.empty()) {
            set_modal(false);
            hide();
        }
    }

    return !filter;
}

} // namespace boken
