#include "utility.hpp"
#include <cstdarg>

namespace boken {

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

} //namespace boken
