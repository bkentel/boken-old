#pragma once

#include "bkassert/assert.hpp"

#include <type_traits>
#include <utility>

namespace boken {

struct valid_t {};

constexpr valid_t valid {};

template <typename T>
class maybe {
    using Tag = std::conditional_t<
        std::is_reference<T>::value, std::false_type, std::true_type>;

    auto&& get_(std::true_type) const noexcept {
        return reinterpret_cast<T const&>(data_);
    }

    auto&& get_(std::true_type) noexcept {
        return reinterpret_cast<T&>(data_);
    }

    auto&& get_(std::false_type) const noexcept {
        return **reinterpret_cast<std::remove_const_t<std::remove_reference_t<T>> const**>(&data_);
    }

    auto&& get_(std::false_type) noexcept {
        return **reinterpret_cast<std::remove_reference_t<T>**>(&data_);
    }

    auto&& value_() noexcept {
        return get_(Tag {});
    }

    auto&& value_() const noexcept {
        return get_(Tag {});
    }

    template <typename... Args>
    void allocate_(std::true_type, Args&&... args) noexcept {
        new (&data_) T {std::forward<Args>(args)...};
    }

    template <typename U>
    void allocate_(std::false_type, U&& arg) noexcept {
        new (&data_) std::remove_reference_t<T>* {&arg};
    }

    void destroy_(std::true_type) noexcept {
        reinterpret_cast<T&>(data_).~T();
    }

    void destroy_(std::false_type) noexcept {
    }
public:
    maybe() noexcept : state_ {state_t::empty} {}

    maybe(std::nullptr_t) noexcept : state_ {state_t::empty} {}

    template <typename... Args>
    maybe(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
      : state_ {state_t::ok}
    {
        allocate_(Tag {}, std::forward<Args>(args)...);
    }

    maybe(maybe&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
      : state_ {other.state_}
    {
        if (state_ == state_t::ok) {
            allocate_(Tag {}, std::move(other.value_()));
        }
    }

    ~maybe() {
        if (state_ == state_t::ok) {
            destroy_(Tag {});
        }
    }

    void release() noexcept {
        state_ = state_t::empty;
    }

    operator T&&() && noexcept {
        BK_ASSERT_OPT(state_ == state_t::ok);
        state_ = state_t::empty;
        return std::move(value_());
    }

    operator T const&() const& noexcept {
        BK_ASSERT_OPT(state_ == state_t::ok);
        return value_();
    }

    explicit operator bool() const noexcept { return state_ == state_t::ok; }

    template <typename F>
    maybe const& operator&&(F f) const {
        if (state_ == state_t::ok) {
            f(value_());
        }

        return *this;
    }

    template <typename F>
    maybe& operator&&(F f) {
        if (state_ == state_t::ok) {
            f(static_cast<std::add_rvalue_reference_t<T>>(value_()));
            state_ = state_t::was_ok;
        }

        return *this;
    }

    template <typename F>
    maybe const& operator||(F f) const {
        if (state_ == state_t::empty) {
            f();
        }

        return *this;
    }

    template <typename F>
    maybe& operator||(F f) {
        if (state_ == state_t::empty) {
            f();
        }

        return *this;
    }

    bool operator||(valid_t) const noexcept {
        return state_ != state_t::empty;
    }

    bool operator&&(valid_t) const noexcept {
        return state_ != state_t::empty;
    }
private:
    std::aligned_storage_t<
        std::is_reference<T>::value ? sizeof(std::remove_reference_t<T>*)  : sizeof(T)
      , std::is_reference<T>::value ? alignof(std::remove_reference_t<T>*) : alignof(T)
    > data_;

    enum class state_t : int8_t {
        empty, ok, was_ok
    };

    state_t state_;
};


} // namespace boken
