#pragma once

#include "config.hpp"

#include <array>
#include <string>

#include <cstdint>
#include <cstddef>
#include <cstdarg>

namespace boken {

namespace detail {

bool static_string_buffer_append(
    char const* fmt
  , char*       buffer
  , ptrdiff_t&  offset
  , size_t      size
  , va_list     args) noexcept;

} // namespace detail

class string_buffer_base {
public:
    string_buffer_base(char* const data, size_t const capacity) noexcept
      : first_    {0}
      , data_     {data}
      , capacity_ {static_cast<ptrdiff_t>(capacity)}
    {
    }

    explicit operator bool() const noexcept {
        return first_ >= 0 && first_ < capacity_ - 1;
    }

#ifndef _MSC_VER
#   define BK_PRINTF_ATTRIBUTE __attribute__ ((__format__(__printf__, 2, 3)))
#else
#   define BK_PRINTF_ATTRIBUTE
#endif

    bool append(char const* const fmt, ...) noexcept BK_PRINTF_ATTRIBUTE;

#undef BK_PRINTF_ATTRIBUTE

    void clear() noexcept {
        first_   = 0;
        data_[0] = '\0';
    }

    bool full()  const noexcept { return first_ >= capacity_ - 1; }
    bool empty() const noexcept { return first_ == 0; }
    size_t capacity() const noexcept { return static_cast<size_t>(capacity_); }
    size_t size() const noexcept { return static_cast<size_t>(first_); }

    char const* begin() const noexcept { return data_; }
    char*       begin()       noexcept { return data_; }

    char const* end() const noexcept { return data_ + first_; }
    char*       end()       noexcept { return data_ + first_; }

    char const* data() const noexcept { return data_; }
    char*       data()       noexcept { return data_; }

    string_view to_string_view() const noexcept {
        return string_view {data_, static_cast<size_t>(first_)};
    }

    std::string to_string() const {
        return std::string {data_, static_cast<size_t>(first_)};
    }
private:
    ptrdiff_t first_;
    char*     data_;
    ptrdiff_t capacity_;
};

namespace detail {

template <size_t N>
struct static_string_buffer_base {
    std::array<char, N> buffer_;
};

} // namespace detail

template <size_t N>
class static_string_buffer
  : public  string_buffer_base
  , private detail::static_string_buffer_base<N>
{
    static_assert(N > 0, "");
    static constexpr auto last_index = static_cast<ptrdiff_t>(N - 1);
public:
    static_string_buffer(static_string_buffer const&) = delete;
    static_string_buffer& operator=(static_string_buffer const&) = delete;

    static_string_buffer(static_string_buffer&&) = default;
    static_string_buffer& operator=(static_string_buffer&&) = default;

    static_string_buffer() noexcept
      : string_buffer_base {detail::static_string_buffer_base<N>::buffer_.data(), N}
    {
    }
};

} // namespace boken
