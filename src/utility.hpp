#pragma once

#include <type_traits>
#include <algorithm>
#include <array>
#include <memory>
#include <cstddef>
#include <cstdint>

namespace boken {

namespace detail {
template <typename Container, typename Compare>
void sort_impl(Container&& c, Compare comp) {
    using std::sort;
    using std::begin;
    using std::end;

    sort(begin(c), end(c), comp);
}
} //namespace detail

namespace container_algorithms {

template <typename Container, typename Compare>
void sort(Container&& c, Compare comp) {
    detail::sort_impl(std::forward<Container>(c), comp);
}

template <typename Container, typename Predicate>
auto find_if(Container&& c, Predicate pred) {
    using std::begin;
    using std::end;

    return std::find_if(begin(c), end(c), pred);
}

template <typename Container, typename Predicate>
auto* find_ptr_if(Container&& c, Predicate pred) {
    using std::begin;
    using std::end;

    using result_t = decltype(std::addressof(*begin(c)));
    auto const it = find_if(std::forward<Container>(c), pred);
    return (it == end(c))
      ? static_cast<result_t>(nullptr)
      : std::addressof(*it);
}

} // container_algorithms

enum class convertion_type {
    unchecked, clamp, fail, modulo
};

template <convertion_type T>
using convertion_t = std::integral_constant<convertion_type, T>;

template <typename T>
constexpr auto as_unsigned(T const n, convertion_t<convertion_type::clamp>) noexcept {
    return static_cast<std::make_unsigned_t<T>>(n < 0 ? 0 : n);
}

template <typename T>
constexpr auto as_unsigned(T const n, convertion_type const type = convertion_type::clamp) noexcept {
    static_assert(std::is_arithmetic<T>::value, "");
    using ct = convertion_type;
    return type == ct::clamp ? as_unsigned(n, convertion_t<ct::clamp> {})
                             : as_unsigned(n, convertion_t<ct::clamp> {});
}

template <typename T, typename U>
inline constexpr ptrdiff_t check_offsetof() noexcept {
    static_assert(std::is_standard_layout<T>::value
                , "Must be standard layout");
    static_assert(!std::is_function<U>::value
                , "Must not be a function");
    static_assert(!std::is_member_function_pointer<std::add_pointer_t<U>>::value
                , "Must not be a member function");
    return 0;
}

#define BK_OFFSETOF(s, m) (check_offsetof<s, decltype(s::m)>() + offsetof(s, m))

template <typename T>
inline constexpr std::add_const_t<T>& as_const(T& t) noexcept {
    return t;
}

template <typename T>
inline constexpr std::add_const_t<T>* as_const(T* const t) noexcept {
    return t;
}

template <typename T>
inline constexpr void call_destructor(T& t)
    noexcept(std::is_nothrow_destructible<T>::value)
{
    t.~T();
}

template <size_t StackSize>
class basic_buffer {
public:
    explicit basic_buffer(size_t const size)
      : data_  {}
      , size_  {size <= StackSize ? StackSize : size}
      , first_ {init_storage_(data_, size_)}
    {
    }

    basic_buffer() noexcept
      : basic_buffer {StackSize}
    {
    }

    ~basic_buffer() {
        size_ <= StackSize ? call_destructor(data_.s_)
                           : call_destructor(data_.d_);
    }

    auto size()  const noexcept { return size_; }
    auto data()  const noexcept { return as_const(first_); }
    auto begin() const noexcept { return data(); }
    auto end()   const noexcept { return begin() + size(); }
    auto data()        noexcept { return first_; }
    auto begin()       noexcept { return data(); }
    auto end()         noexcept { return begin() + size(); }

    char operator[](size_t const i) const noexcept {
        return *(begin() + i);
    }

    char& operator[](size_t const i) noexcept {
        return *(begin() + i);
    }
private:
    using static_t  = std::array<char, StackSize + 1>;
    using dynamic_t = std::unique_ptr<char[]>;

    union storage_t {
        storage_t() noexcept {}
        ~storage_t() {}

        static_t  s_;
        dynamic_t d_;
    } data_;

    static char* init_storage_(storage_t& data, size_t const size) {
        if (size && size <= StackSize) {
            new (&data.s_) static_t;
            return data.s_.data();
        } else {
            new (&data.d_) dynamic_t {new char[size]};
            return data.d_.get();
        }
    }

    size_t size_;
    char*  first_;
};

using dynamic_buffer = basic_buffer<0>;

template <size_t Size>
using static_buffer = basic_buffer<Size>;

} //namespace boken
