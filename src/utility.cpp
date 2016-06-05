#include "utility.hpp"
#include "format.hpp"
#include <cstdarg>

namespace boken {

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#endif

bool detail::static_string_buffer_append(
    char const* const fmt
  , char*       const buffer
  , ptrdiff_t&        offset
  , size_t      const size
  , va_list           args
) noexcept {
    auto const last = static_cast<ptrdiff_t>(size) - 1;

    if (offset < 0 || offset >= last) {
        return false;
    }

    auto const n = vsnprintf(
        buffer + offset
      , size - static_cast<size_t>(offset)
      , fmt
      , args);

    if (n < 0) {
        return false; //error
    } else if (offset + n > last) {
        offset = last;
        return false; //overflow
    }

    offset += n;
    return true; // ok
}

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

bool string_buffer_base::append(char const* const fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);

    auto const result = detail::static_string_buffer_append(
        fmt, data_, first_, static_cast<size_t>(capacity_), args);

    va_end(args);

    return result;
}

} //namespace boken
