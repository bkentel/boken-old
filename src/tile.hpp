#pragma once

#include "config.hpp"
#include "hash.hpp"
#include "math_types.hpp"
#include "types.hpp"

#include <bitset>
#include <type_traits>
#include <unordered_map>

#include <cstdint>

namespace boken {

enum class tile_id : uint32_t {
    invalid = 0
  , empty  = djb2_hash_32c("empty")
  , floor  = djb2_hash_32c("floor")
  , tunnel = djb2_hash_32c("tunnel")
  , wall_0000 = djb2_hash_32c("wall_0000") //!< ____
  , wall_0001 = djb2_hash_32c("wall_0001") //!< ___S
  , wall_0010 = djb2_hash_32c("wall_0010") //!< __E_
  , wall_0011 = djb2_hash_32c("wall_0011") //!< __ES
  , wall_0100 = djb2_hash_32c("wall_0100") //!< _W__
  , wall_0101 = djb2_hash_32c("wall_0101") //!< _W_S
  , wall_0110 = djb2_hash_32c("wall_0110") //!< _WE_
  , wall_0111 = djb2_hash_32c("wall_0111") //!< _WES
  , wall_1000 = djb2_hash_32c("wall_1000") //!< N___
  , wall_1001 = djb2_hash_32c("wall_1001") //!< N__S
  , wall_1010 = djb2_hash_32c("wall_1010") //!< N_E_
  , wall_1011 = djb2_hash_32c("wall_1011") //!< N_ES
  , wall_1100 = djb2_hash_32c("wall_1100") //!< NW__
  , wall_1101 = djb2_hash_32c("wall_1101") //!< NW_S
  , wall_1110 = djb2_hash_32c("wall_1110") //!< NWE_
  , wall_1111 = djb2_hash_32c("wall_1111") //!< NWES
  , door_ns_closed = djb2_hash_32c("door_ns_closed")
  , door_ns_open   = djb2_hash_32c("door_ns_open")
  , door_ew_closed = djb2_hash_32c("door_ew_closed")
  , door_ew_open   = djb2_hash_32c("door_ew_open")
  , stair_down = djb2_hash_32c("stair_down")
  , stair_up   = djb2_hash_32c("stair_up")
};

template <typename Enum>
Enum string_to_enum(string_view str) noexcept;

string_view enum_to_string(tile_id id) noexcept;

enum class tile_type : uint16_t {
    empty, wall, floor, tunnel, door, stair
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

    template <size_t Bit>
    constexpr explicit tile_flags(flag_t<Bit>) noexcept
      : tile_flags {Bit}
    {
    }

    bool none() const noexcept {
        return bits_.none();
    }

    template <size_t Bit>
    constexpr bool test(flag_t<Bit>) const noexcept {
        return bits_.test(Bit - 1);
    }

    template <size_t Bit>
    void clear(flag_t<Bit>) noexcept {
        bits_.reset(Bit - 1);
    }
private:
    std::bitset<32> bits_;
};

struct tile_data {
    uint64_t data;
};

//should be sorted by alignment for packing
struct tile_data_set {
    tile_data  data;
    tile_flags flags;
    tile_id    id;
    tile_type  type;
    region_id  rid;
};

enum class tile_map_type : uint32_t {
    base, entity, item
};

class tile_map {
public:
    tile_map(
        tile_map_type type
      , uint32_t      texture_id
      , sizei32x      tile_w
      , sizei32y      tile_h
      , sizei32x      tiles_x
      , sizei32y      tiles_y
    ) noexcept;

    recti32 index_to_rect(uint32_t const i) const noexcept {
        auto const tx = value_cast_unsafe<uint32_t>(tiles_x_);
        auto const p  = point2i32 {
            value_cast(tile_w_) * static_cast<int32_t>(i % tx)
          , value_cast(tile_h_) * static_cast<int32_t>(i / tx)};

        return {p, tile_w_, tile_h_};
    }

    sizei32x tile_width()  const noexcept { return tile_w_; }
    sizei32y tile_height() const noexcept { return tile_h_; }

    sizei32x width()  const noexcept { return tiles_x_; }
    sizei32y height() const noexcept { return tiles_y_; }

    uint32_t texture_id() const noexcept { return texture_id_; }

    //TODO remove these
    template <typename T, typename Tag>
    void add_mapping(tagged_value<T, Tag> const id, uint32_t const index) {
        mappings_.insert(std::make_pair(value_cast(id), index));
    }

    template <typename T, typename Tag>
    uint32_t find(tagged_value<T, Tag> const id) const noexcept {
        auto const it = mappings_.find(value_cast(id));
        return it == std::end(mappings_) ? 0 : it->second;
    }
private:
    tile_map_type type_;
    uint32_t      texture_id_ {0};

    sizei32x tile_w_;
    sizei32y tile_h_;
    sizei32x tiles_x_;
    sizei32y tiles_y_;

    std::unordered_map<uint32_t, uint32_t> mappings_;
};

uint32_t id_to_index(tile_map const& map, tile_id id) noexcept;
uint32_t id_to_index(tile_map const& map, entity_id id) noexcept;
uint32_t id_to_index(tile_map const& map, item_id id) noexcept;

} //namespace boken
