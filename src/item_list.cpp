#include "item_list.hpp"
#include "system.hpp"
#include "scope_guard.hpp"
#include "command.hpp"
#include "item_pile.hpp"

#include "bkassert/assert.hpp"

namespace boken {

//--------------------------------------------------------------------------
item_list_controller::item_list_controller(std::unique_ptr<inventory_list> list)
  : list_ {std::move(list)}
{
    reset_callbacks();
}

void item_list_controller::add_column(std::string heading, std::function<std::string(item const&)> getter) {
    auto const id = static_cast<uint8_t>(list_->cols() & 0xFFu);
    list_->add_column(id, std::move(heading), std::move(getter));
}

//--------------------------------------------------------------------------
void item_list_controller::on_confirm(on_confirm_t handler) {
    on_confirm_ = std::move(handler);
}

void item_list_controller::on_cancel(on_cancel_t handler) {
    on_cancel_ = std::move(handler);
}

void item_list_controller::reset_callbacks() {
    on_confirm_ = [](auto, auto) noexcept {};
    on_cancel_  = [&]() noexcept { hide(); };
}

//--------------------------------------------------------------------------
bool item_list_controller::on_key(kb_event const& event, kb_modifiers const& kmods) {
    auto& il = *list_;

    if (!is_visible()) {
        return !is_modal();
    }

    return true;
}

bool item_list_controller::on_text_input(text_input_event const& event) {
    auto& il = *list_;

    if (!is_visible()) {
        return !is_modal();
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
                auto const p = il.get_selection();
                on_confirm_(&hit_test.y, &hit_test.y + 1);
                return !is_visible();
            }

            if (kmods.test(kb_modifiers::m_shift)) {
                il.selection_toggle(hit_test.y);
            } else {
                il.selection_set({hit_test.y});
            }
        }
    }

    return false;
}

bool item_list_controller::on_mouse_move(mouse_event const& event, kb_modifiers const& kmods) {
    using type  = inventory_list::hit_test_result::type;
    using btype = mouse_event::button_change_t;

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
    if (!hit_test) {
        return !is_modal();
    }

    // indicate the row the mouse is over
    if (hit_test.what == type::cell && event.button_state_bits() == 0) {
        il.indicate(hit_test.y);
    }

    return false;
}

bool item_list_controller::on_mouse_wheel(int const wy, int const wx, kb_modifiers const& kmods) {
    auto& il = *list_;

    if (!is_visible()) {
        return !is_modal();
    }

    return true;
}

bool item_list_controller::on_command(command_type const type, uint64_t const data) {
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

    if (type == command_type::move_n) {
        il.indicate_prev();
    } else if (type == command_type::move_s) {
        il.indicate_next();
    } else if (type == command_type::cancel) {
        on_cancel_();
        return !is_visible();
    } else if (type == command_type::confirm) {
        if (is_multi_select_) {
            auto const sel = il.get_selection();
            if (sel.first != sel.second) {
                on_confirm_(sel.first, sel.second);
            } else {
                on_cancel_();
            }
        } else {
            auto const i = il.indicated();
            on_confirm_(&i, &i + 1);
        }
        return !is_visible();
    } else if (type == command_type::toggle) {
        if (is_multi_select_) {
            il.selection_toggle(il.indicated());
        }
    }

    return false;
}

//--------------------------------------------------------------------------

//! clears all state and row data; leaves columns intact.
void item_list_controller::clear() {
    auto& il = *list_;

    il.clear_rows();
    il.selection_clear();
}

void item_list_controller::assign(item_pile const& items) {
    clear();

    auto& il = *list_;

    il.reserve(il.cols(), items.size());

    std::for_each(begin(items), end(items), [&](item_instance_id const id) {
        il.add_row(id);
    });

    il.layout();
}

void item_list_controller::append(item_instance_id const id) {
    list_->add_row(id);
}

void item_list_controller::layout() {
    list_->layout();
}

void item_list_controller::set_title(std::string title) {
    list_->set_title(std::move(title));
}

//--------------------------------------------------------------------------
bool item_list_controller::set_modal(bool const state) noexcept {
    BK_ASSERT(!state || state && is_visible());
    bool const result = is_modal_;
    is_modal_ = state;
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

//--------------------------------------------------------------------------
void item_list_controller::show() noexcept {
    set_visible_(true);
}

void item_list_controller::hide() noexcept {
    set_visible_(false);
}

//! @returns the visible state of the list after toggling.
bool item_list_controller::toogle_visible() noexcept {
    bool const is_visible = list_->is_visible();
    set_visible_(!is_visible);
    return !is_visible;
}

bool item_list_controller::is_visible() const noexcept {
    return list_->is_visible();
}

void item_list_controller::set_visible_(bool const state) noexcept {
    is_moving_ = false;
    is_sizing_ = false;
    state ? list_->show() : list_->hide();
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

} // namespace boken
