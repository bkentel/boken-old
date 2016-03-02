#pragma once

#include "config.hpp"
#include "hash.hpp"
#include "math.hpp"
#include "types.hpp"

#include <bkassert/assert.hpp>

#include <bitset>
#include <type_traits>
#include <unordered_map>

#include <cstdint>

namespace boken {

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4307) // integral constant overflow
#endif

enum class tile_id : uint32_t {
    invalid = 0
  , empty  = djb2_hash_32c("empty")
  , floor  = djb2_hash_32c("floor")
  , tunnel = djb2_hash_32c("tunnel")
  , wall_0000 = djb2_hash_32c("wall_0000")
  , wall_0001 = djb2_hash_32c("wall_0001")
  , wall_0010 = djb2_hash_32c("wall_0010")
  , wall_0011 = djb2_hash_32c("wall_0011")
  , wall_0100 = djb2_hash_32c("wall_0100")
  , wall_0101 = djb2_hash_32c("wall_0101")
  , wall_0110 = djb2_hash_32c("wall_0110")
  , wall_0111 = djb2_hash_32c("wall_0111")
  , wall_1000 = djb2_hash_32c("wall_1000")
  , wall_1001 = djb2_hash_32c("wall_1001")
  , wall_1010 = djb2_hash_32c("wall_1010")
  , wall_1011 = djb2_hash_32c("wall_1011")
  , wall_1100 = djb2_hash_32c("wall_1100")
  , wall_1101 = djb2_hash_32c("wall_1101")
  , wall_1110 = djb2_hash_32c("wall_1110")
  , wall_1111 = djb2_hash_32c("wall_1111")
  , door_ns_closed = djb2_hash_32c("door_ns_closed")
  , door_ns_open   = djb2_hash_32c("door_ns_open")
  , door_ew_closed = djb2_hash_32c("door_ew_closed")
  , door_ew_open   = djb2_hash_32c("door_ew_open")
};

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

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
    uint16_t   region_id;
};

enum class tile_map_type : uint32_t {
    base, entity, item
};

class tile_map {
public:
    tile_map(
        tile_map_type const type
      , uint32_t      const texture_id
      , sizeix        const tile_w
      , sizeiy        const tile_h
      , sizeix        const tiles_x
      , sizeiy        const tiles_y
    ) noexcept
      : type_       {type}
      , texture_id_ {texture_id}
      , tile_w_     {tile_w}
      , tile_h_     {tile_h}
      , tiles_x_    {tiles_x}
      , tiles_y_    {tiles_y}
    {
        BK_ASSERT_SAFE(tile_w  > sizeix {0});
        BK_ASSERT_SAFE(tile_h  > sizeiy {0});
        BK_ASSERT_SAFE(tiles_x > sizeix {0});
        BK_ASSERT_SAFE(tiles_y > sizeiy {0});
    }

    recti index_to_rect(uint32_t const i) const noexcept {
        auto const tx = value_cast<uint32_t>(tiles_x_);
        auto const tw = value_cast(tile_w_);
        auto const th = value_cast(tile_h_);
        auto const x  = static_cast<int32_t>(i % tx);
        auto const y  = static_cast<int32_t>(i / tx);

        return {offix {x * tw}, offiy {y * th}, tile_w_, tile_h_};
    }

    sizeix tile_width()  const noexcept { return tile_w_; }
    sizeiy tile_height() const noexcept { return tile_h_; }

    sizeix width()  const noexcept { return tiles_x_; }
    sizeiy height() const noexcept { return tiles_y_; }

    uint32_t texture_id() const noexcept { return texture_id_; }

    //TODO remove these
    template <typename T, typename Tag>
    void add_mapping(tagged_integral_value<T, Tag> const id, uint32_t const index) {
        mappings_.insert(std::make_pair(value_cast(id), index));
    }

    template <typename T, typename Tag>
    uint32_t find(tagged_integral_value<T, Tag> const id) const noexcept {
        auto const it = mappings_.find(value_cast(id));
        return it == std::end(mappings_) ? 0 : it->second;
    }
private:
    tile_map_type type_;
    uint32_t      texture_id_ {0};

    sizeix tile_w_;
    sizeiy tile_h_;
    sizeix tiles_x_;
    sizeiy tiles_y_;

    std::unordered_map<uint32_t, uint32_t> mappings_;
};

uint32_t id_to_index(tile_map const& map, tile_id id) noexcept;
uint32_t id_to_index(tile_map const& map, entity_id id) noexcept;

} //namespace boken
