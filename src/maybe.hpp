#pragma once

#include "bkassert/assert.hpp"

#include <type_traits>
#include <utility>

namespace boken {

struct valid_t {};
struct empty_t {};

constexpr valid_t valid {};
constexpr empty_t empty {};

template <typename T> class maybe;

template <typename T>
T value_or(maybe<T>&& value, T&& fallback);

template <typename T>
T require(maybe<T>&& value);

template <typename T, typename F, typename R>
R result_of_or(maybe<T>&& value, R&& fallback, F f);

template <typename T>
struct maybe_base {
    template <typename... Args>
    void allocate(Args&&... args)
        noexcept(std::is_nothrow_constructible<T, Args...>::value)
    {
        ::new (&data) T {std::forward<Args>(args)...};
    }

    void destroy() noexcept(std::is_nothrow_destructible<T>::value) {
        rvalue().~T();
    }

    std::remove_cv_t<T> value() const noexcept {
        return reinterpret_cast<T&>(&data);
    }

    T&& rvalue() noexcept {
        return std::move(reinterpret_cast<T&>(data));
    }

    T& rvalue() const noexcept {
        return reinterpret_cast<T&>(data);
    }

    std::aligned_storage_t<sizeof(T), alignof(T)> data;
};

template <typename T>
struct maybe_base<T&> {
    template <typename U
      , typename = std::enable_if_t<std::is_convertible<U&, T&>::value>>
    void allocate(U&& ref) noexcept {
        ::new (&data) T* {std::addressof(ref)};
    }

    void destroy() noexcept {}

    std::remove_cv_t<T> value() const noexcept {
        return **reinterpret_cast<T**>(&data);
    }

    std::remove_cv_t<T> const& rvalue() const noexcept {
        return **reinterpret_cast<T**>(&data);
    }

    T& rvalue() noexcept {
        return **reinterpret_cast<T**>(&data);
    }

    std::aligned_storage_t<sizeof(T*), alignof(T*)> data;
};

template <typename T>
class maybe : private maybe_base<T> {
    friend T value_or<T>(maybe<T>&&, T&&);

    friend T require<T>(maybe<T>&&);

    template <typename U, typename F, typename R>
    friend R result_of_or(maybe<U>&&, R&&, F);

    using base = maybe_base<T>;
public:
    maybe(std::nullptr_t) noexcept {}

    template <typename... Args>
    maybe(Args&&... args)
        noexcept(std::is_nothrow_constructible<T, Args...>::value)
    {
        base::allocate(std::forward<Args>(args)...);
        has_value_ = true;
    }

    maybe(maybe&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        if (other.has_value_) {
            base::allocate(other.rvalue());
            has_value_ = true;
        }
    }

    ~maybe() noexcept(std::is_nothrow_destructible<T>::value)
    {
        if (has_value_) {
            base::destroy();
        }
    }

    explicit operator bool() const noexcept { return has_value_; }

    template <typename F>
    maybe&& operator>>(F f) &&
        noexcept(std::is_nothrow_move_constructible<T>::value
              && noexcept(f(std::declval<T>())))
    {
        if (has_value_) {
            f(base::rvalue());
        }

        return std::move(*this);
    }

    template <typename U>
    maybe& operator>>(U&&) & = delete;

    template <typename U>
    maybe& operator>>(U&&) const& = delete;

    template <typename F>
    bool operator||(F f) const noexcept(noexcept(f)) {
        if (has_value_) {
            return true;
        }

        f();
        return false;
    }

    bool operator&&(valid_t) const noexcept { return has_value_; }
    bool operator&&(empty_t) const noexcept { return !has_value_; }
private:
    bool has_value_ {false};
};

template <typename T, typename... Args>
maybe<T> make_maybe(Args&&... args) {
    return {std::forward<Args>(args)...};
}

template <typename T>
maybe<T> make_maybe(T&& value) {
    return {std::forward<T>(value)};
}

template <typename T>
T value_or(maybe<T>&& value, T&& fallback) {
    if (!value) {
        return std::move(fallback);
    }

    return {value.rvalue()};
}

template <typename T>
T require(maybe<T>&& value) {
    if (!value) {
        std::terminate();
    }

    return {value.rvalue()};
}

template <typename T, typename F, typename R>
R result_of_or(maybe<T>&& value, R&& fallback, F f) {
    return value ? f(value.rvalue())
                 : R {std::move(fallback)};
}

} // namespace boken
