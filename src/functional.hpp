#pragma once

namespace boken {

#include <type_traits>
#include <utility>

//! Type trait for the number of parameters a function (object) takes.
template <typename F>
struct arity_of;

template <typename R, typename... Args>
struct arity_of<R (*)(Args...)> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct arity_of<R (C::*)(Args...)> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct arity_of<R (C::*)(Args...) const> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename R, typename... Args>
struct arity_of<R (Args...)> {
    static constexpr size_t value = sizeof...(Args);
};

template <typename F>
struct arity_of {
    static_assert(std::is_class<std::decay_t<F>>::value, "");
    static constexpr size_t value = arity_of<decltype(&F::operator())>::value;
};

template <bool Result, typename F>
struct void_as_bool_t {
    F f_;

    template <bool Noexcept, typename... Args>
    bool invoke(std::true_type, Args&&... args) const noexcept(Noexcept) {
        return f_(std::forward<Args>(args)...);
    }

    template <bool Noexcept, typename... Args>
    bool invoke(std::false_type, Args&&... args) const noexcept(Noexcept) {
        return f_(std::forward<Args>(args)...), Result;
    }

    template <typename... Args>
    bool operator()(Args&&... args) const
        noexcept(noexcept(f_(std::forward<Args>(args)...)))
    {
        constexpr bool is_noexcept = noexcept(f_(std::forward<Args>(args)...));

        using R = std::decay_t<decltype(f_(std::forward<Args>(args)...))>;
        using Tag =
            std::conditional_t<std::is_void<R>::value,       std::false_type
    	  , std::conditional_t<std::is_same<bool, R>::value, std::true_type
                                                           , void>>;

        static_assert(!std::is_void<Tag>::value, "");

        return invoke<is_noexcept>(Tag {}, std::forward<Args>(args)...);
    }
};

template <bool Result, typename F>
auto void_as_bool(F&& f) noexcept {
  return void_as_bool_t<Result, F> {std::forward<F>(f)};
}

} // namespace boken
