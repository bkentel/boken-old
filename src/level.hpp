#pragma once

#include "types.hpp"
#include <memory>
#include <bitset>
#include <type_traits>

namespace boken {

class item;
class entity;

struct tag_tile_id {};
using tile_id = tagged_integral_value<uint16_t, tag_tile_id>;

struct tile_flags {
    template <size_t Bit>
    using flag_t = std::integral_constant<size_t, Bit>;

    static constexpr flag_t<1> f_solid = flag_t<1> {};

    constexpr explicit tile_flags(uint32_t const n = 0) noexcept
      : bits_ {n}
    {
    }

    bool none() const noexcept {
        return bits_.none();
    }

    template <size_t Bit>
    constexpr bool test(flag_t<Bit>) const noexcept {
        return bits_.test(Bit - 1);
    }

    std::bitset<32> bits_;
};

struct tile_data {
    uint64_t data;
};

struct tile_view {
    tile_id    const&       id;
    tile_flags const&       flags;
    uint16_t   const&       tile_index;
    tile_data  const* const data;
};

//====---
// A generic level concept
//====---
class level {
public:
    virtual ~level() = default;

    virtual item   const* find(item_instance_id   id) const noexcept = 0;
    virtual entity const* find(entity_instance_id id) const noexcept = 0;

    virtual tile_view at(int x, int y) const noexcept = 0;
};

std::unique_ptr<level> make_level(sizeix width, sizeiy height);

} //namespace boken
