#pragma once

#include "math.hpp"
#include "rect.hpp"
#include "tile.hpp"

#include <cstdint>
#include <cstddef>

namespace boken {

enum class tribool {
    no, maybe, yes
};

namespace detail {
using tile_type_reader = std::integral_constant<int, 0>;
using tile_id_reader   = std::integral_constant<int, 1>;

} // namespace detail

//!
//!
//!
template <typename Reader>
auto make_simple_reader(detail::tile_type_reader, Reader read) noexcept {
    return [=](point2i32 const p) noexcept { return read.tile_type_at(p); };
}

//!
//!
//!
template <typename Reader>
auto make_simple_reader(detail::tile_id_reader, Reader read) noexcept {
    return [=](point2i32 const p) noexcept { return read.tile_id_at(p); };
}

//!
//!
//!
template <typename T, T Head, T... Tail>
bool match_any_of(T const n) noexcept {
    constexpr std::array<T, 1u + sizeof...(Tail)> ts {Head, Tail...};
    return std::find(ts.begin(), ts.end(), n) != ts.end();
}

//!
//!
//!
template <tile_type T0, tile_type... Ts, typename Reader>
auto match_any_of(Reader reader) noexcept {
    auto const read = make_simple_reader(detail::tile_type_reader {}, reader);
    return [read](point2i32 const p) noexcept {
        return match_any_of<tile_type, T0, Ts...>(read(p));
    };
}

//!
//!
//!
template <tile_id T0, tile_id... Ts, typename Reader>
auto match_any_of(Reader reader) noexcept {
    auto const read = make_simple_reader(detail::tile_id_reader {}, reader);
    return [read](point2i32 const p) noexcept {
        return match_any_of<tile_id, T0, Ts...>(read(p));
    };
}

//!
//!
//!
template <typename T>
auto make_bounds_checker(axis_aligned_rect<T> const bounds) noexcept {
    return [=](point2<T> const p) noexcept {
        return intersects(bounds, p);
    };
}

//!
//!
//!
template <typename Reader, typename Checker>
uint32_t get_wall_neighbors(point2i32 const p, Reader read, Checker check) noexcept {
    return fold_neighbors4(
        p, check, match_any_of<tile_type::wall, tile_type::door>(read));
}

//!
//!
//!
tile_id wall_type_from_neighbors(uint32_t neighbors) noexcept;

//!
//!
//!
template <typename Reader, typename Checker>
tile_id wall_to_id_at(point2i32 const p, Reader read, Checker check) noexcept {
    auto const id = read.tile_id_at(p);
    if (id != tile_id::invalid) {
        return id;
    }

    auto const neighbors = get_wall_neighbors(p, read, check);
    return wall_type_from_neighbors(neighbors);
}

//!
//!
//!
template <typename Reader, typename Checker>
tile_id get_id_at(point2i32 const p, Reader read, Checker check) noexcept {
    using ti = tile_id;
    using tt = tile_type;

    auto const type = read.tile_type_at(p);

    switch (type) {
    case tt::empty :
        return ti::empty;
    case tt::floor :
        return ti::floor;
    case tt::tunnel :
        return ti::tunnel;
    case tt::door :
        break;
    case tt::stair :
        break;
    case tt::wall :
        return wall_to_id_at(p, read, check);
    default :
        break;
    }

    return ti::invalid;
}

//!
//!
//!
template <typename Reader, typename Checker>
bool can_omit_wall_at(point2i32 const p, Reader read, Checker check) noexcept {
    using tt = tile_type;

    auto const walls  = fold_neighbors8(p, check, match_any_of<tt::wall>(read));
    auto const floors = fold_neighbors8(p, check, match_any_of<tt::floor>(read));

    // [#][#][#]
    // [?][#][?]
    // [?][.][?]
    if (
        ((walls & 0b111'00'000) == 0b111'00'000) && (floors & 0b000'00'010)
    ) {
        return true;
    }

    // [?][?][#]
    // [.][#][#]
    // [?][?][#]
    if (
        ((walls & 0b001'01'001) == 0b001'01'001) && (floors & 0b000'10'000)
    ) {
        return true;
    }

    return false;
}

//!
//!
//!
bool can_gen_tunnel_at_wall(uint32_t neighbors) noexcept;

//!
//!
//!
template <typename T, typename Read, typename Check>
bool can_gen_tunnel_at_wall(point2<T> const p, Read read, Check check) noexcept {
    return can_gen_tunnel_at_wall(
        fold_neighbors4(p, check, match_any_of<tile_type::wall>(read)));
}

//!
//!
//!
tribool can_gen_tunnel(tile_type type) noexcept;

//!
//!
//!
bool is_door_candidate(tile_type type) noexcept;

//!
//!
//!
template <typename Reader, typename Checker>
tile_id try_place_door_at(point2i32 const p, Reader read, Checker check) noexcept {
    BK_ASSERT(check(p));

    using tt = tile_type;
    using ti = tile_id;

    auto const type = read.tile_type_at(p);
    if (!is_door_candidate(type)) {
        return ti::invalid;
    }

    auto const walls  = fold_neighbors4(p, check, match_any_of<tt::wall>(read));
    auto const spaces = fold_neighbors4(p, check
        , match_any_of<tt::floor, tt::tunnel, tt::stair>(read));

    if (walls == 0b1001 && spaces == 0b0110) {
        // NS door
        auto const matcher = match_any_of<
            ti::wall_1001, ti::wall_1011, ti::wall_1101, ti::wall_1111>(read);

        if (matcher(p + vec2i32 {0, -1}) && matcher(p + vec2i32 {0, 1})) {
            return ti::door_ns_closed;
        }
    } else if (walls == 0b0110 && spaces == 0b1001) {
        // EW door
        auto const matcher = match_any_of<
            ti::wall_0110, ti::wall_0111, ti::wall_1110, ti::wall_1111>(read);

        if (matcher(p + vec2i32 {-1, 0}) && matcher(p + vec2i32 {1, 0})) {
            return ti::door_ew_closed;
        }
    }

    return ti::invalid;
}

//!
//!
//!
template <typename Reader, typename Checker>
bool can_gen_tunnel_at(point2i32 const p, Reader read, Checker check) noexcept {
    BK_ASSERT(check(p));

    auto const type   = read.tile_type_at(p);
    auto const result = can_gen_tunnel(type);
    if (result != tribool::maybe) {
        return result == tribool::yes;
    }

    switch (type) {
    case tile_type::wall :
        return can_gen_tunnel_at_wall(p, read, check);
    case tile_type::empty :
    case tile_type::floor :
    case tile_type::tunnel :
    case tile_type::door :
    case tile_type::stair :
    default :
        BK_ASSERT(false);
        break;
    }

    return false;
}

} // namespace boken
