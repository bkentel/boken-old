#pragma once

#include "types.hpp"

#include <memory>
#include <functional>
#include <array>
#include <bitset>
#include <type_traits>
#include <vector>

namespace boken {

struct kb_modifiers {
    template <size_t Bit>
    using flag_t = std::integral_constant<size_t, Bit>;

    template <size_t... Bits>
    using flags_t = std::integer_sequence<size_t, Bits...>;

    static constexpr flag_t<1>  m_left_shift  = flag_t<1> {};
    static constexpr flag_t<2>  m_right_shift  = flag_t<2> {};
    static constexpr flag_t<3>  m_left_ctrl  = flag_t<3> {};
    static constexpr flag_t<4>  m_right_ctrl  = flag_t<4> {};
    static constexpr flag_t<5>  m_left_alt  = flag_t<5> {};
    static constexpr flag_t<6>  m_right_alt  = flag_t<6> {};
    static constexpr flag_t<7>  m_left_gui  = flag_t<7> {};
    static constexpr flag_t<8>  m_right_gui  = flag_t<8> {};
    static constexpr flag_t<9>  m_num  = flag_t<9> {};
    static constexpr flag_t<10> m_caps  = flag_t<10> {};
    static constexpr flag_t<11> m_mode  = flag_t<11> {};

    static constexpr flags_t<m_left_shift, m_right_shift> m_shift  = flags_t<m_left_shift, m_right_shift> {};
    static constexpr flags_t<m_left_ctrl, m_right_ctrl> m_ctrl  = flags_t<m_left_ctrl, m_right_ctrl> {};
    static constexpr flags_t<m_left_alt, m_right_alt> m_alt  = flags_t<m_left_alt, m_right_alt> {};
    static constexpr flags_t<m_left_gui, m_right_gui> m_gui  = flags_t<m_left_gui, m_right_gui> {};

    constexpr explicit kb_modifiers(uint32_t const n) noexcept
      : bits_ {n}
    {
    }

    bool none() const noexcept {
        return bits_.none();
    }

    template <size_t Bit>
    constexpr bool test(flag_t<Bit>) const noexcept {
        return bits_.test(Bit - 1);
    }

    template <size_t Bit0, size_t Bit1>
    constexpr bool test(flags_t<Bit0, Bit1>) const noexcept {
        return bits_.test(Bit0 - 1) || bits_.test(Bit1 - 1);
    }

    std::bitset<16> bits_;
};

struct kb_event {
    uint32_t timestamp;
    uint32_t scancode;
    uint32_t keycode;
    uint16_t mods;
    bool     is_repeat;
    bool     went_down;
};

struct read_only_pointer_t {
    read_only_pointer_t() noexcept = default;

    template <typename T>
    read_only_pointer_t(
        T const* const beg
      , T const* const end
      , size_t   const stride = sizeof(T)
    ) noexcept
      : ptr            {static_cast<void const*>(beg)}
      , last           {static_cast<void const*>(end)}
      , element_size   {sizeof(T)}
      , element_stride {static_cast<uint16_t>(stride)}
    {
    }

    template <typename T>
    explicit read_only_pointer_t(std::vector<T> const& v, size_t const offset = 0, size_t const stride = sizeof(T)) noexcept
      : read_only_pointer_t {
            reinterpret_cast<T const*>(
                reinterpret_cast<char const*>(
                    reinterpret_cast<void const*>(v.data())
                ) + offset)
          , v.data() + v.size()
          , stride}
    {
    }

    read_only_pointer_t& operator++() noexcept {
        ptr = (ptr == last) ? ptr
          : reinterpret_cast<char const*>(ptr) + element_stride;
        return *this;
    }

    read_only_pointer_t operator++(int) noexcept {
        auto result = *this;
        ++(*this);
        return result;
    }

    template <typename T>
    T const& value() const noexcept {
        return *reinterpret_cast<T const*>(ptr);
    }

    explicit operator bool() const noexcept {
        return !!ptr;
    }

    void const* ptr            {};
    void const* last           {};
    uint16_t    element_size   {};
    uint16_t    element_stride {};
};

enum class render_data_type {
    position // x, y
  , texture  // x, y
  , color    // rgba
};

struct mouse_event {
    static constexpr size_t button_count = 4;

    enum class button_change_t : uint8_t {
        none, went_up, went_down
    };

    int x;
    int y;
    int dx;
    int dy;

    std::array<button_change_t, button_count> button_change;
    std::array<bool,            button_count> button_state;
};

class system {
public:
    virtual ~system() = default;

    using on_request_quit_handler = std::function<bool ()>;
    virtual void on_request_quit(on_request_quit_handler handler) = 0;

    using on_key_handler = std::function<void (kb_event, kb_modifiers)>;
    virtual void on_key(on_key_handler handler) = 0;

    using on_mouse_move_handler = std::function<void (mouse_event, kb_modifiers)>;
    virtual void on_mouse_move(on_mouse_move_handler handler) = 0;

    using on_mouse_button_handler = std::function<void (mouse_event, kb_modifiers)>;
    virtual void on_mouse_button(on_mouse_button_handler handler) = 0;

    using on_mouse_wheel_handler = std::function<void (int wy, int wx, kb_modifiers)>;
    virtual void on_mouse_wheel(on_mouse_wheel_handler handler) = 0;

    virtual bool is_running() = 0;
    virtual int do_events() = 0;

    virtual void render_clear()   = 0;
    virtual void render_present() = 0;

    virtual void render_set_tile_size(int w, int h) = 0;
    virtual void render_set_transform(float sx, float sy, float tx, float ty) = 0;

    virtual void render_set_data(render_data_type type, read_only_pointer_t data) = 0;
    virtual void render_data_n(int n) = 0;
};

std::unique_ptr<system> make_system();

} //namespace boken
