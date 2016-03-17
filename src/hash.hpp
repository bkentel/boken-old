#pragma once

#include <cstdint>
#include <cstddef>

namespace boken {

constexpr inline uint32_t djb2_hash_32c(
    uint32_t    const hash
  , char const* const s
  , ptrdiff_t   const i
) noexcept {
    return i
        ? djb2_hash_32c(
            static_cast<uint32_t>(((static_cast<uint64_t>(hash) << 5) + hash + static_cast<uint8_t>(*s)) & 0xFFFFFFFFu)
          , s + 1
          , i - 1)
        : hash;
}

template <size_t N>
constexpr inline uint32_t djb2_hash_32c(char const (&s)[N]) noexcept {
    return djb2_hash_32c(5381u, s, N - 1);
}

template <typename It, typename Predicate>
inline uint32_t djb2_hash_32(It const first, Predicate pred) noexcept {
    uint32_t hash = 5381u;

    for (auto it = first; pred(it); ++it) {
        hash = (hash << 5) + hash + static_cast<uint8_t>(*it);
    }

    return hash;
}

template <typename It>
inline uint32_t djb2_hash_32(It const first, It const last) noexcept {
    return djb2_hash_32(first
      , [&](It const it) noexcept { return it != last; });
}

inline uint32_t djb2_hash_32(char const* s) noexcept {
    return djb2_hash_32(s
      , [&](char const* const it) noexcept { return !!*it; });
}

} //namespace boken
