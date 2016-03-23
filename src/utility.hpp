#pragma once

#include "config.hpp" //string_view

#include <bkassert/assert.hpp>

#include <type_traits>
#include <algorithm>
#include <array>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <cstdarg>

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
void as_const(T const&&) = delete;

template <typename T>
inline constexpr std::add_const_t<T>* as_const(T* const t) noexcept {
    return t;
}

template <typename T>
inline constexpr void call_destructor(T& t)
    noexcept(std::is_nothrow_destructible<T>::value)
{
    t.~T();
    (void)t; // spurious unused warning
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

template <typename T>
class sub_region_iterator : public std::iterator_traits<T*> {
    using this_t = sub_region_iterator<T>;
    template <typename U> friend class sub_region_iterator;
public:
    using reference = typename std::iterator_traits<T*>::reference;
    using pointer   = typename std::iterator_traits<T*>::pointer;

    sub_region_iterator(
        T* const p
      , ptrdiff_t const off_x,       ptrdiff_t const off_y
      , ptrdiff_t const width_outer, ptrdiff_t const height_outer
      , ptrdiff_t const width_inner, ptrdiff_t const height_inner
      , ptrdiff_t const x = 0,       ptrdiff_t const y = 0
    ) noexcept
      : p_ {p + (off_x + x) + (off_y + y) * width_outer}
      , off_x_ {off_x}
      , off_y_ {off_y}
      , width_outer_ {width_outer}
      , width_inner_ {width_inner}
      , height_inner_ {height_inner}
      , x_ {x}
      , y_ {y}
    {
        BK_ASSERT(!!p);
        BK_ASSERT(off_x >= 0 && off_y >= 0);
        BK_ASSERT(width_inner >= 0 && width_outer >= width_inner + off_x);
        BK_ASSERT(height_inner >= 0 && height_outer >= height_inner + off_y);
        BK_ASSERT(x_ <= width_inner && y_ <= height_inner);
    }

    // create a new iterator with the same properties as other, but with a
    // different base pointer.
    template <typename U>
    sub_region_iterator(sub_region_iterator<U> it, T* const p) noexcept
      : p_ {p + (it.off_x_ + it.x_) + (it.off_y_ + it.y_) * it.width_outer_}
      , off_x_ {it.off_x_}
      , off_y_ {it.off_y_}
      , width_outer_ {it.width_outer_}
      , width_inner_ {it.width_inner_}
      , height_inner_ {it.height_inner_}
      , x_ {it.x_}
      , y_ {it.y_}
    {
    }

    reference operator*() const noexcept {
        return *p_;
    }

    pointer operator->() const noexcept {
        return &**this;
    }

    void operator++() noexcept {
        ++p_;
        if (++x_ < width_inner_) {
            return;
        }

        if (++y_ < height_inner_) {
            x_ = 0;
            p_ += (width_outer_ - width_inner_);
        }
    }

    sub_region_iterator operator++(int) noexcept {
        auto result = *this;
        ++(*this);
        return result;
    }

    bool operator<(this_t const& other) const noexcept {
        return p_ < other.p_;
    }

    bool operator==(this_t const& other) const noexcept {
        return (p_ == other.p_);
    }

    bool operator!=(this_t const& other) const noexcept {
        return !(*this == other);
    }

    ptrdiff_t operator-(this_t const& other) const noexcept {
        BK_ASSERT(is_compatible_(other));

        return (x_       + y_       * width_inner_)
             - (other.x_ + other.y_ * other.width_inner_);
    }

    ptrdiff_t x()      const noexcept { return x_; }
    ptrdiff_t y()      const noexcept { return y_; }
    ptrdiff_t off_x()  const noexcept { return off_x_; }
    ptrdiff_t off_y()  const noexcept { return off_y_; }
    ptrdiff_t width()  const noexcept { return width_inner_; }
    ptrdiff_t height() const noexcept { return height_inner_; }
    ptrdiff_t stride() const noexcept { return width_outer_; }
private:
    template <typename U>
    bool is_compatible_(sub_region_iterator<U> const& it) const noexcept {
        return off_x_        == it.off_x_
            && off_y_        == it.off_y_
            && width_outer_  == it.width_outer_
            && width_inner_  == it.width_inner_
            && height_inner_ == it.height_inner_;
    }

    T* p_ {};

    ptrdiff_t off_x_ {};
    ptrdiff_t off_y_ {};
    ptrdiff_t width_outer_ {};
    ptrdiff_t width_inner_ {};
    ptrdiff_t height_inner_ {};

    ptrdiff_t x_ {};
    ptrdiff_t y_ {};
};

template <typename T>
using const_sub_region_iterator = sub_region_iterator<std::add_const_t<std::decay_t<T>>>;

template <typename T>
using sub_region_range = std::pair<
    sub_region_iterator<T>, sub_region_iterator<T>>;

template <typename T>
using const_sub_region_range = sub_region_range<std::add_const_t<std::decay_t<T>>>;

template <typename T>
sub_region_range<T> make_sub_region_range(
    T* const p
  , ptrdiff_t const off_x,       ptrdiff_t const off_y
  , ptrdiff_t const width_outer, ptrdiff_t const height_outer
  , ptrdiff_t const width_inner, ptrdiff_t const height_inner
) noexcept {
    return {
        sub_region_iterator<T> {
            p
          , off_x, off_y
          , width_outer, height_outer
          , width_inner, height_inner
        }
      , sub_region_iterator<T> {
            p
          , off_x, off_y
          , width_outer, height_outer
          , width_inner, height_inner
          , width_inner, height_inner - 1
      }
    };
}

namespace detail {
#ifndef _MSC_VER
#   define BK_PRINTF_ATTRIBUTE __attribute__ ((__format__(__printf__, 1, 5)))
#else
#   define BK_PRINTF_ATTRIBUTE
#endif

bool static_string_buffer_append(
    char const* fmt
  , char*       buffer
  , ptrdiff_t&  offset
  , size_t      size
  , va_list     args) noexcept;
#undef BK_PRINTF_ATTRIBUTE

} // namespace detail

template <size_t N>
class static_string_buffer {
    static_assert(N > 0, "");
    static constexpr auto last_index = static_cast<ptrdiff_t>(N - 1);
public:
    static_string_buffer() noexcept
      : first_ {0}
      , buffer_()
    {
    }

    explicit operator bool() const noexcept {
        return first_ >= 0 && first_ < last_index;
    }

    bool append(char const* const fmt, ...) noexcept {
        va_list args;
        va_start(args, fmt);

        auto const result = detail::static_string_buffer_append(
            fmt, buffer_.data(), first_, N, args);

        va_end(args);

        return result;
    }

    void clear() noexcept {
        first_ = 0;
        buffer_[0] = '\0';
    }

    bool full()  const noexcept { return first_ >= last_index; }
    bool empty() const noexcept { return first_ == 0; }
    size_t capacity() const noexcept { return N; }
    size_t size() const noexcept { return static_cast<size_t>(first_); }

    char const* begin() const noexcept { return buffer_.data(); }
    char*       begin()       noexcept { return buffer_.data(); }

    char const* end() const noexcept { return buffer_.data() + first_; }
    char*       end()       noexcept { return buffer_.data() + first_; }

    char const* data() const noexcept { return buffer_.data(); }
    char*       data()       noexcept { return buffer_.data(); }

    string_view to_string_view() const noexcept {
        return string_view {buffer_.data(), static_cast<size_t>(first_)};
    }

    std::string to_string() const {
        return std::string {buffer_.data(), static_cast<size_t>(first_)};
    }
private:
    ptrdiff_t first_;
    std::array<char, N> buffer_;
};

} //namespace boken
