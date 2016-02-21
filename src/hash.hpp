#pragma once

#include <cstdint>
#include <cstddef>

namespace boken {

constexpr inline uint32_t djb2_hash_32c(
    uint32_t    const hash
  , char const* const s
  , ptrdiff_t   const i
) noexcept {
    return i ? djb2_hash_32c((hash << 5) + hash
                          + static_cast<uint8_t>(s[i]), s, i - 1)
             : hash;
}

template <size_t N>
constexpr inline uint32_t djb2_hash_32c(char const (&s)[N]) noexcept {
    return djb2_hash_32c(5381u, s, N - 1);
}

constexpr inline uint32_t djb2_hash_32c(
    uint32_t    const hash
  , char const* const s
) noexcept {
    return *s ? djb2_hash_32c((hash << 5) + hash
                           + static_cast<uint8_t>(*s), s + 1)
              : hash;
}

inline uint32_t djb2_hash_32(char const* s) noexcept {
    uint32_t hash = 5381u;

    for (; *s; ++s) {
        hash = (hash << 5) + hash + static_cast<uint8_t>(*s);
    }

    return hash;
}

} //namespace boken
