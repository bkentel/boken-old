#pragma once

#include "types.hpp"
#include "math.hpp"
#include <bitset>
#include <type_traits>
#include <cstdint>

namespace boken {

enum class tile_type : uint16_t {
    empty, wall, floor, door, stair
};

class tile_flags {
public:
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
private:
    std::bitset<32> bits_;
};

struct tile_data {
    uint64_t data;
};

struct tile_map {
    recti index_to_rect(int32_t const i) const noexcept {
        return {
            offix {i % value_cast(tiles_x)}
          , offiy {i / value_cast(tiles_x)}
          , tile_w
          , tile_h
        };
    }

    uint16_t id_to_index(tile_id const id) const noexcept {
        return value_cast(id);
    }

    sizeix tile_w  {18};
    sizeiy tile_h  {18};
    sizeix tiles_x {16};
    sizeiy tiles_y {16};
};

} //namespace boken
