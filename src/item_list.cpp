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

//--------------------------------------------------------------------------
item_list_controller::item_list_controller(std::unique_ptr<inventory_list> list)
  : list_ {std::move(list)}
{
    set_on_command();
    set_on_focus_change();
    set_on_selection_change();
}

void item_list_controller::set_properties(
    std::string title
  , std::initializer_list<flag_type> const flags
) {
    set_title(std::move(title));

    auto const has_flag = [&] {
        using type = std::underlying_type_t<flag_type>;
        type const value = std::accumulate(begin(flags), end(flags), uint32_t {0}
          , [](type const sum, auto const f) noexcept {
                return sum | static_cast<type>(f);
            });

        return [=](flag_type const flag) noexcept {
            return !!(value & static_cast<type>(flag));
        };
    }();

    set_visible(has_flag(flag_type::visible));
    set_modal(has_flag(flag_type::modal));
    set_multiselect(has_flag(flag_type::multiselect));
}

void item_list_controller::add_column(std::string heading, get_f getter, sort_f sorter) {
    auto const id = static_cast<uint8_t>(list_->cols() & 0xFFu);
    list_->add_column(id, std::move(heading), std::move(getter), std::move(sorter));
}

void item_list_controller::add_column(std::string heading, get_f getter) {
    auto const by_string = [](
        const_item_descriptor, string_view const lhs
      , const_item_descriptor, string_view const rhs
    ) noexcept {
        return lhs.compare(rhs);
    };

    add_column(std::move(heading), std::move(getter), by_string);
}

//! add a standard column
void item_list_controller::add_column(
    const_context const ctx
  , column_type   const type
) {
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

void item_list_controller::add_columns(
    const_context      const ctx
  , column_type const* const first
  , column_type const* const last
) {
    for (auto it = first; it != last; ++it) {
        add_column(ctx, *it);
    }
}

void item_list_controller::add_columns(
    const_context const ctx
  , std::initializer_list<column_type> const list
) {
    add_columns(ctx, list.begin(), list.end());
}

//--------------------------------------------------------------------------
void item_list_controller::set_on_command(on_command_t handler) {
    if (!is_processing_callback_) {
        if (on_command_) {
            command_stack_.push_back(std::move(on_command_));
        }

        on_command_ = std::move(handler);
        on_command_(command_type::none);
    } else {
        BK_ASSERT(!on_command_swap_);
        on_command_swap_ = std::move(handler);
    }
}

void item_list_controller::set_on_command() {
    BK_ASSERT(!is_processing_callback_);
    on_command_ = [](auto) noexcept { return event_result::pass_through; };
}

void item_list_controller::set_on_focus_change(on_focus_change_t handler) {
    on_focus_change_ = std::move(handler);
}

void item_list_controller::set_on_focus_change() {
    on_focus_change_ = [](auto) noexcept {};
}

void item_list_controller::set_on_selection_change(on_selection_change_t handler) {
    on_selection_change_ = std::move(handler);
}

void item_list_controller::set_on_selection_change() {
    on_selection_change_ = [](auto) noexcept {};
}

//--------------------------------------------------------------------------

void item_list_controller::do_on_selection_change_(int const prev_sel) {
    auto& il = *list_;

    auto const i = il.indicated();
    if (i == prev_sel) {
        return;
    }

    il.scroll_into_view(0, i);
    on_selection_change_(i);
}

//--------------------------------------------------------------------------

bool item_list_controller::on_key(kb_event const& event, kb_modifiers const& kmods) {
    if (!is_visible()) {
        return true;
    }

    return true;
}

bool item_list_controller::on_text_input(text_input_event const& event) {
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

bool item_list_controller::on_mouse_button(mouse_event const& event, kb_modifiers const& kmods) {
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

bool item_list_controller::on_mouse_move(mouse_event const& event, kb_modifiers const& kmods) {
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

bool item_list_controller::on_mouse_wheel(int const wy, int const wx, kb_modifiers const& kmods) {
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

event_result item_list_controller::do_on_command_(command_type const type) {
    // the temporary should always be empty here
    BK_ASSERT(!is_processing_callback_ && !on_command_swap_);

    is_processing_callback_ = true;
    auto on_exit = BK_SCOPE_EXIT {
        is_processing_callback_ = false;
    };

    return on_command_(type);
}

bool item_list_controller::on_command(command_type const type, uint64_t const data) {
    auto on_exit = BK_SCOPE_EXIT {
        // there should never be a queued handler when we finish
        BK_ASSERT(!on_command_swap_);
    };

    auto& il = *list_;

    if (!is_visible()) {
        return !is_modal();
    }

    auto const hit_test = il.hit_test(last_mouse_);

    // passthrough the command if the mouse isn't over the list
    // unless the list is modal.
    if (!hit_test && !is_modal()) {
        return true;
    }

    auto const result = [&] {
        using ct = command_type;

        if (type == ct::move_n) {
            do_on_selection_change_(il.indicate_prev());
            return event_result::filter;
        } else if (type == ct::move_s) {
            do_on_selection_change_(il.indicate_next());
            return event_result::filter;
        } else if (type == ct::toggle) {
            if (is_multi_select_) {
                il.selection_toggle(il.indicated());
            }
            return event_result::filter;
        }

        return do_on_command_(type);
    }();

    auto const pass = !is_modal();

    auto const on_detach = [&](bool const detach) {
        if (on_command_swap_) {
            std::swap(on_command_swap_, on_command_);
            if (!detach) {
                command_stack_.push_back(std::move(on_command_swap_));
            }

            on_command_swap_ = on_command_t {};
            on_command_(command_type::none);
            return;
        }

        if (!detach) {
            return;
        }

        // The default handler should never not be on the stack if a user
        // handler is being detached
        BK_ASSERT(!command_stack_.empty());

        on_command_ = std::move(command_stack_.back());
        command_stack_.pop_back();
        on_command_(command_type::none);

        if (command_stack_.empty()) {
            set_modal(false);
            hide();
        }
    };

    switch (result) {
    case event_result::filter:              on_detach(false); return false;
    case event_result::filter_detach:       on_detach(true);  return false;
    case event_result::pass_through:        on_detach(false); return pass;
    case event_result::pass_through_detach: on_detach(true);  return pass;
    default:
        BK_ASSERT(false);
        break;
    }

    return pass;
}

//--------------------------------------------------------------------------

//! clears all state and row data; leaves columns intact.
void item_list_controller::clear() {
    auto& il = *list_;

    il.clear_rows();
    il.selection_clear();
}

int item_list_controller::assign(item_pile const& items) {
    clear();

    auto& il = *list_;

    il.reserve(il.cols(), items.size());

    std::for_each(begin(items), end(items), [&](item_instance_id const id) {
        il.add_row(id);
    });

    il.layout();

    return static_cast<int>(il.rows());
}

void item_list_controller::append(item_instance_id const id) {
    list_->add_row(id);
}

void item_list_controller::append(std::initializer_list<item_instance_id> const list) {
    list_->add_rows(begin(list), end(list));
}

void item_list_controller::remove_rows(
    int const* const first
  , int const* const last
) {
    list_->remove_rows(first, last);
}

void item_list_controller::layout() {
    list_->layout();
}

void item_list_controller::set_title(std::string title) {
    list_->set_title(std::move(title));
}

//--------------------------------------------------------------------------
bool item_list_controller::has_selection() const noexcept {
    //TODO: a bit wasteful
    return !!list_->get_selection().first;
}

bool item_list_controller::set_modal(bool const state) noexcept {
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

bool item_list_controller::is_modal() const noexcept {
    return is_modal_;
}

bool item_list_controller::set_multiselect(bool const state) noexcept {
    bool const result = is_multi_select_;
    is_multi_select_ = state;
    return result;
}

bool item_list_controller::is_multiselect() const noexcept {
    return is_multi_select_;
}

bool item_list_controller::has_focus() const noexcept {
    return is_visible() && (is_modal() || list_->hit_test(last_mouse_));
}

//--------------------------------------------------------------------------
void item_list_controller::show() noexcept {
    set_visible(true);
}

void item_list_controller::hide() noexcept {
    set_visible(false);
}

//! @returns the visible state of the list after toggling.
bool item_list_controller::toggle_visible() noexcept {
    bool const is_visible = list_->is_visible();
    set_visible(!is_visible);
    return !is_visible;
}

bool item_list_controller::is_visible() const noexcept {
    return list_->is_visible();
}

void item_list_controller::set_visible(bool const state) noexcept {
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

void item_list_controller::resize_(point2i32 const p, vec2i32 const v) {
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

bool item_list_controller::cancel() noexcept {
    if (has_selection()) {
        list_->selection_clear();
        return false;
    }

    if (is_modal()) {
        set_modal(false);
        return false;
    }

    return true;
}

bool item_list_controller::cancel_force() noexcept {
    while (!cancel()) {}
    return true;
}

} // namespace boken
