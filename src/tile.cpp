#include "tile.hpp"

#include <bkassert/assert.hpp>

namespace boken {

tile_map::tile_map(
    tile_map_type const type
  , uint32_t      const texture_id
  , sizei32x      const tile_w
  , sizei32y      const tile_h
  , sizei32x      const tiles_x
  , sizei32y      const tiles_y
) noexcept
  : type_       {type}
  , texture_id_ {texture_id}
  , tile_w_     {tile_w}
  , tile_h_     {tile_h}
  , tiles_x_    {tiles_x}
  , tiles_y_    {tiles_y}
{
    BK_ASSERT_SAFE(value_cast(tile_w)  > 0);
    BK_ASSERT_SAFE(value_cast(tile_h)  > 0);
    BK_ASSERT_SAFE(value_cast(tiles_x) > 0);
    BK_ASSERT_SAFE(value_cast(tiles_y) > 0);
}

template <>
tile_id string_to_enum<tile_id>(string_view const str) noexcept {
    auto const hash = djb2_hash_32(str.data());

    #define BK_ENUM_MAPPING(x) case tile_id::x : return tile_id::x
    switch (static_cast<tile_id>(hash)) {
        BK_ENUM_MAPPING(empty);
        BK_ENUM_MAPPING(floor);
        BK_ENUM_MAPPING(tunnel);
        BK_ENUM_MAPPING(wall_0000);
        BK_ENUM_MAPPING(wall_0001);
        BK_ENUM_MAPPING(wall_0010);
        BK_ENUM_MAPPING(wall_0011);
        BK_ENUM_MAPPING(wall_0100);
        BK_ENUM_MAPPING(wall_0101);
        BK_ENUM_MAPPING(wall_0110);
        BK_ENUM_MAPPING(wall_0111);
        BK_ENUM_MAPPING(wall_1000);
        BK_ENUM_MAPPING(wall_1001);
        BK_ENUM_MAPPING(wall_1010);
        BK_ENUM_MAPPING(wall_1011);
        BK_ENUM_MAPPING(wall_1100);
        BK_ENUM_MAPPING(wall_1101);
        BK_ENUM_MAPPING(wall_1110);
        BK_ENUM_MAPPING(wall_1111);
        BK_ENUM_MAPPING(door_ns_closed);
        BK_ENUM_MAPPING(door_ns_open);
        BK_ENUM_MAPPING(door_ew_closed);
        BK_ENUM_MAPPING(door_ew_open);
        BK_ENUM_MAPPING(stair_down);
        BK_ENUM_MAPPING(stair_up);
        case tile_id::invalid:
            break;
        default:
            break;
    }
    #undef BK_ENUM_MAPPING

    return tile_id::invalid;
}

string_view enum_to_string(tile_id const id) noexcept {
    #define BK_ENUM_MAPPING(x) case tile_id::x : return #x
    switch (id) {
        BK_ENUM_MAPPING(empty);
        BK_ENUM_MAPPING(floor);
        BK_ENUM_MAPPING(tunnel);
        BK_ENUM_MAPPING(wall_0000);
        BK_ENUM_MAPPING(wall_0001);
        BK_ENUM_MAPPING(wall_0010);
        BK_ENUM_MAPPING(wall_0011);
        BK_ENUM_MAPPING(wall_0100);
        BK_ENUM_MAPPING(wall_0101);
        BK_ENUM_MAPPING(wall_0110);
        BK_ENUM_MAPPING(wall_0111);
        BK_ENUM_MAPPING(wall_1000);
        BK_ENUM_MAPPING(wall_1001);
        BK_ENUM_MAPPING(wall_1010);
        BK_ENUM_MAPPING(wall_1011);
        BK_ENUM_MAPPING(wall_1100);
        BK_ENUM_MAPPING(wall_1101);
        BK_ENUM_MAPPING(wall_1110);
        BK_ENUM_MAPPING(wall_1111);
        BK_ENUM_MAPPING(door_ns_closed);
        BK_ENUM_MAPPING(door_ns_open);
        BK_ENUM_MAPPING(door_ew_closed);
        BK_ENUM_MAPPING(door_ew_open);
        BK_ENUM_MAPPING(stair_down);
        BK_ENUM_MAPPING(stair_up);
        case tile_id::invalid:
            break;
        default:
            break;
    }
    #undef BK_ENUM_MAPPING

    return "invalid tile_id";
}

uint32_t id_to_index(tile_map const& map, tile_id const id) noexcept {
    using ti = tile_id;

    auto const tx = value_cast(map.width());
    auto const to_index = [&](int const x, int const y) noexcept {
        return static_cast<uint32_t>(x + y * tx);
    };

    //NWES
    switch (id) {
    case ti::empty:     return to_index(11, 13);
    case ti::floor:     return to_index( 7,  0);
    case ti::tunnel:    return to_index(10, 15);
    case ti::wall_0000: return to_index( 0, 15); // none
    case ti::wall_0001: return to_index( 2, 13); // south
    case ti::wall_0010: return to_index( 6, 12); // east
    case ti::wall_0011: return to_index( 9, 12); // se
    case ti::wall_0100: return to_index( 5, 11); // west
    case ti::wall_0101: return to_index(11, 11); // sw
    case ti::wall_0110: return to_index(13, 12); // ew
    case ti::wall_0111: return to_index(11, 12); // esw
    case ti::wall_1000: return to_index( 0, 13); // n
    case ti::wall_1001: return to_index(10, 11); // ns
    case ti::wall_1010: return to_index( 8, 12); // ne
    case ti::wall_1011: return to_index(12, 12); // nes
    case ti::wall_1100: return to_index(12, 11); // nw
    case ti::wall_1101: return to_index( 9, 11); // nsw
    case ti::wall_1110: return to_index(10, 12); // new
    case ti::wall_1111: return to_index(14, 12); // nesw
    case ti::door_ns_closed: return to_index( 3, 11);
    case ti::door_ns_open:   return to_index( 6, 11);
    case ti::door_ew_closed: return to_index( 4, 12);
    case ti::door_ew_open:   return to_index( 6, 11);
    case ti::stair_down: return to_index( 14, 3);
    case ti::stair_up:   return to_index( 12, 3);
    case ti::invalid:
        break;
    default:
        break;
    }

    return 0;
}

uint32_t id_to_index(tile_map const& map, entity_id const id) noexcept {
    return map.find(id);
}

uint32_t id_to_index(tile_map const& map, item_id const id) noexcept {
    return map.find(id);
}

} //namespace boken
