#pragma once

namespace boken {

namespace detail {
struct scope_guard_tag         {};
struct scope_guard_success_tag {};
struct scope_guard_failed_tag  {};
} // namespace detail

template <typename F>
class scope_guard {
public:
    scope_guard(scope_guard const&) = delete;
    void* operator new(size_t) = delete;

    scope_guard(scope_guard&& other)
      : function_  {std::move(other.function_)}
      , dismissed_ {other.dismissed_}
    {
        other.dismissed_ = true;
    }

    ~scope_guard() {
        if (!dismissed_) {
            function_();
        }
    }

    template <typename Fn>
    explicit scope_guard(Fn&& fn)
      : function_ {std::forward<Fn>(fn)}
    {
    }
private:
    F    function_;
    bool dismissed_ {false};

};

#if (__cplusplus > 201406L) || (_MSC_VER >= 1900)

template <typename F, bool Execute>
class scope_guard_new_exception {
public:
    scope_guard_new_exception(scope_guard_new_exception const&) = delete;
    void* operator new(size_t) = delete;

    scope_guard_new_exception(scope_guard_new_exception&&) = default;
    scope_guard_new_exception& operator=(scope_guard_new_exception&&) = default;

    ~scope_guard_new_exception() noexcept(Execute) {
        if (Execute == (count_ != std::uncaught_exceptions())) {
            function_();
        }
    }

    template <typename Fn>
    explicit scope_guard_new_exception(Fn&& fn)
      : function_ {std::forward<Fn>(fn)}
      , count_    {std::uncaught_exceptions()}
    {
    }
private:
    F   function_;
    int count_;
};

#endif

template <typename F>
auto operator+(detail::scope_guard_tag, F&& f) {
    return scope_guard<std::decay_t<F>> {std::forward<F>(f)};
}

} //namespace boken

#define BK_SCOPE_EXIT ::boken::detail::scope_guard_tag {} + [&]() noexcept
