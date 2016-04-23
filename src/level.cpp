#include "level.hpp"

#include "algorithm.hpp"
#include "bsp_generator.hpp"    // for bsp_generator, etc
#include "random.hpp"           // for random_state (ptr only), etc
#include "random_algorithm.hpp"
#include "tile.hpp"             // for tile_data_set, tile_type, tile_flags, etc
#include "utility.hpp"          // for find_if
#include "item_pile.hpp"
#include "spatial_map.hpp"
#include "rect.hpp"
#include "graph.hpp"

#include "forward_declarations.hpp"

#include <bkassert/assert.hpp>  // for BK_ASSERT

#include <algorithm>            // for max, find_if, fill, max_element, min, etc
#include <functional>           // for reference_wrapper, ref
#include <iterator>             // for begin, end, back_insert_iterator, etc
#include <numeric>
#include <vector>               // for vector

#include <cstdint>              // for uint16_t, int32_t

// TODO make testing possible
namespace boken {

tile_id wall_type_from_neighbors(uint32_t const neighbors) noexcept {
    using ti = tile_id;

    switch (neighbors) {
    case 0b0000 : return ti::wall_0000;
    case 0b0001 : return ti::wall_0001;
    case 0b0010 : return ti::wall_0010;
    case 0b0011 : return ti::wall_0011;
    case 0b0100 : return ti::wall_0100;
    case 0b0101 : return ti::wall_0101;
    case 0b0110 : return ti::wall_0110;
    case 0b0111 : return ti::wall_0111;
    case 0b1000 : return ti::wall_1000;
    case 0b1001 : return ti::wall_1001;
    case 0b1010 : return ti::wall_1010;
    case 0b1011 : return ti::wall_1011;
    case 0b1100 : return ti::wall_1100;
    case 0b1101 : return ti::wall_1101;
    case 0b1110 : return ti::wall_1110;
    case 0b1111 : return ti::wall_1111;
    default     : break;
    }

    return ti::invalid;
}

template <typename T, typename Read, typename Check>
bool can_omit_wall_at(point2<T> const p, Read read, Check check) noexcept {
    auto const is_wall = [&](point2<T> const q) noexcept {
        return read(q) == tile_type::wall;
    };

    auto const is_floor = [&](point2<T> const q) noexcept {
        return read(q) == tile_type::floor;
    };

    auto const wall_type  = fold_neighbors8(p, check, is_wall);
    auto const other_type = fold_neighbors8(p, check, is_floor);

    // north
    if (
        ((wall_type & 0b111'00'000) == 0b111'00'000) && (other_type & 0b000'00'010)
    ) {
        return true;
    }

    // east
    if (
        ((wall_type & 0b001'01'001) == 0b001'01'001) && (other_type & 0b000'10'000)
    ) {
        return true;
    }

    return false;
}

template <typename T, typename Read, typename Check>
tile_id try_place_door_at(point2<T> const p, Read read, Check check) noexcept {
    BK_ASSERT(check(p));

    switch (read(p)) {
    case tile_type::floor:
    case tile_type::tunnel:
    case tile_type::wall:
        break;
    case tile_type::stair:
    case tile_type::empty:
    case tile_type::door:
    default:
        return tile_id::invalid;
    }

    auto const is_wall = [&](point2<T> const q) noexcept {
        return read(q) == tile_type::wall;
    };

    auto const is_connectable = [&](point2<T> const q) noexcept {
        switch (read(q)) {
        case tile_type::floor:
        case tile_type::tunnel:
        case tile_type::stair:
            return true;
        case tile_type::empty:
        case tile_type::wall:
        case tile_type::door:
        default:
            break;
        }

        return false;
    };

    auto const wall_type  = fold_neighbors4(p, check, is_wall);
    auto const other_type = fold_neighbors4(p, check, is_connectable);

    return (wall_type == 0b1001 && other_type == 0b0110) ? tile_id::door_ns_closed
         : (wall_type == 0b0110 && other_type == 0b1001) ? tile_id::door_ew_closed
         : tile_id::invalid;
}

enum class tribool {
    no, maybe, yes
};

//! @note neighbors is the result of fold_neighbors4
bool can_gen_tunnel_at_wall(uint32_t const neighbors) noexcept {
    //                     NWES
    return (neighbors == 0b1001u)
        || (neighbors == 0b0110u);
}

template <typename T, typename Read, typename Check>
bool can_gen_tunnel_at_wall(point2<T> const p, Read read, Check check) noexcept {
    return can_gen_tunnel_at_wall(fold_neighbors4(p, check
      , [&](point2<T> const q) noexcept {
            return read(q) == tile_type::wall;
        }));
}

tribool can_gen_tunnel(tile_type const type) noexcept {
    switch (type) {
    case tile_type::empty  :
    case tile_type::floor  :
    case tile_type::tunnel :
    case tile_type::door   :
    case tile_type::stair  :
        return tribool::yes;
    case tile_type::wall :
        return tribool::maybe;
    default :
        break;
    }

    return tribool::no;
}

template <typename T, typename Read, typename Check>
tribool can_gen_tunnel_at(point2<T> const p, Read read, Check check) noexcept {
    BK_ASSERT(check(p));

    auto const type   = read(p);
    auto const result = can_gen_tunnel(type);

    if (result != tribool::maybe) {
        return result;
    }

    switch (type) {
    case tile_type::wall :
        if (can_gen_tunnel_at_wall(p, read, check)) {
            return tribool::yes;
        }
        break;
    case tile_type::empty  :
    case tile_type::floor  :
    case tile_type::tunnel :
    case tile_type::door   :
    case tile_type::stair  :
    default :
        BK_ASSERT(false);
        break;
    }

    return tribool::no;
}

} // namespace boken

namespace {

template <typename ReadType, typename ReadId, typename Check>
boken::tile_id tile_type_to_id_at(boken::point2i32 const p, ReadType read_type, ReadId read_id, Check check) noexcept {
    using namespace boken;

    static_assert(noexcept(read_type(p)), "");
    static_assert(noexcept(check(p)), "");

    using ti = tile_id;
    using tt = tile_type;

    switch (read_type(p)) {
    case tt::empty  : return ti::empty;
    case tt::floor  : return ti::floor;
    case tt::tunnel : return ti::tunnel;
    case tt::door   : break;
    case tt::stair  : break;
    case tt::wall   :
        {
            auto const id = read_id(p);
            if (id != ti::invalid) {
                return id;
            }
        }

        return wall_type_from_neighbors(
            fold_neighbors4(p, check, [&](point2i32 const q) noexcept {
                auto const type = read_type(q);
                return type == tt::wall || type == tt::door;
            }));
    default : break;
    }

    return ti::invalid;
}

template <typename T>
void fill_rect(
    std::vector<T>& v
  , boken::sizei32x const width
  , boken::recti32  const r
  , T const value
) noexcept {
    using namespace boken;

    auto const x0   = value_cast(r.x0);
    auto const y0   = value_cast(r.y0);
    auto const y1   = value_cast(r.y1);
    auto const w    = value_cast(width);
    auto const rw   = value_cast(r.width());
    auto const step = w - rw;

    auto p = v.data() + x0 + (y0 * w);

    for (auto y = y0; y < y1; ++y) {
        p = std::fill_n(p, rw, value) + step;
    }
}

} //namespace

namespace boken {

level::~level() = default;

struct generate_rect_room {
    generate_rect_room(sizei32 const room_min_size, sizei32 const room_max_size) noexcept
      : room_min_w_ {value_cast(room_min_size)}
      , room_max_w_ {value_cast(room_max_size)}
      , room_min_h_ {value_cast(room_min_size)}
      , room_max_h_ {value_cast(room_max_size)}
    {
    }

    int32_t operator()(random_state& rng, recti32 const area, std::vector<tile_data_set>& out) const {
        auto const r = random_sub_rect(rng, move_to_origin(area)
          , room_min_w_, room_max_w_
          , room_min_h_, room_max_h_);

        auto const w = value_cast(area.width());

        for_each_xy(r, [&](point2i32 const p, bool const on_edge) noexcept {
            auto& data = out[static_cast<size_t>(
                value_cast(p.x) + value_cast(p.y) * w)];

            if (on_edge) {
                data.type  = tile_type::wall;
                data.flags = tile_flags {tile_flags::f_solid};
            } else {
                data.type  = tile_type::floor;
                data.flags = tile_flags {};
            }
        });

        return value_cast(r.area());
    }

    sizei32x room_min_w_;
    sizei32x room_max_w_;

    sizei32y room_min_h_;
    sizei32y room_max_h_;
};

class level_impl : public level {
public:
    level_impl(random_state& rng, world& w, sizei32x const width, sizei32y const height, size_t const id)
      : entities_ {value_cast_unsafe<int16_t>(width), value_cast_unsafe<int16_t>(height)}
      , items_    {value_cast_unsafe<int16_t>(width), value_cast_unsafe<int16_t>(height)}
      , bounds_   {point2i32 {}, width, height}
      , data_     {width, height}
      , world_    {w}
      , id_       {id}
    {
        bsp_generator::param_t p;
        p.width  = sizei32x {width};
        p.height = sizei32y {height};
        p.min_room_size = sizei32 {3};
        p.room_chance_num = sizei32 {80};

        bsp_gen_ = make_bsp_generator(p);
        generate(rng);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // level interface
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    sizei32x width() const noexcept final override {
        return sizei32x {bounds_.width()};
    }

    sizei32y height() const noexcept final override {
        return sizei32y {bounds_.height()};
    }

    recti32 bounds() const noexcept final override {
        return bounds_;
    }

    size_t id() const noexcept final override {
        return id_;
    }

    std::pair<entity*, point2i32>
    find(entity_instance_id const id) noexcept final override {
        auto const result = entities_.find(id);
        if (!result.first) {
            return {nullptr, point2i32 {}};
        }

        return {&boken::find(world_, *result.first), result.second};
    }

    std::pair<entity const*, point2i32>
    find(entity_instance_id const id) const noexcept final override {
        return const_cast<level_impl*>(this)->find(id);
    }

    entity* entity_at(point2i32 const p) noexcept final override {
        auto const id = entities_.find(underlying_cast_unsafe<int16_t>(p));
        return id ? &boken::find(world_, *id) : nullptr;
    }

    entity const* entity_at(point2i32 const p) const noexcept final override {
        return const_cast<level_impl*>(this)->entity_at(p);
    }

    item_pile const* item_at(point2i32 const p) const noexcept final override {
        return items_.find(underlying_cast_unsafe<int16_t>(p));
    }

    placement_result can_place_entity_at(point2i32 const p) const noexcept final override {
        return !check_bounds_(p)
                 ? placement_result::failed_bounds
             : data_at_(data_.flags, p).test(tile_flags::f_solid)
                 ? placement_result::failed_obstacle
             : is_entity_at(p)
                 ? placement_result::failed_entity
                 : placement_result::ok;
    }

    placement_result can_place_item_at(point2i32 const p) const noexcept final override {
        return !check_bounds_(p)
                 ? placement_result::failed_bounds
             : data_at_(data_.flags, p).test(tile_flags::f_solid)
                 ? placement_result::failed_obstacle
                 : placement_result::ok;
    }

    placement_result move_by(item_instance_id const id, vec2i32 const v) noexcept final override {
        return placement_result::ok;
    }

    placement_result move_by(entity_instance_id const id, vec2i32 const v) noexcept final override {
        auto result = placement_result::failed_bad_id;
        entities_.move_to_if(id, [&](entity_instance_id, point2i16 const p) noexcept {
            auto const q = underlying_cast_unsafe<int16_t>(p + v);
            result = can_place_entity_at(q);
            return std::make_pair(q, result == placement_result::ok);
        });

        return result;
    }

    void transform_entities(
        std::function<point2i32 (entity&, point2i32)>&& transform
      , std::function<void (entity&, point2i32, point2i32)>&& on_success
    ) final override {
        auto const values    = entities_.values_range();
        auto const positions = entities_.positions_range();

        auto v_it = values.first;
        auto p_it = positions.first;

        for (size_t i = 0; i < entities_.size(); ++i, ++v_it, ++p_it) {
            auto const p = *p_it;
            auto&      e = boken::find(world_, *v_it);

            auto const q = transform(e, p);
            if (p == q) {
                continue;
            }

            if (move_by(get_instance(e), q - p) == placement_result::ok) {
                on_success(e, p, q);
            }
        }
    }

    item_instance_id add_object_at(unique_item&& i, point2i32 const p) final override {
        auto const result = i.get();

        if (!item_deleter_) {
            item_deleter_ = &i.get_deleter();
        }

        BK_ASSERT(can_place_item_at(p) == placement_result::ok);

        auto const q = underlying_cast_unsafe<int16_t>(p);

        auto* pile = items_.find(q);
        if (!pile) {
            item_pile new_pile;
            new_pile.add_item(std::move(i));
            auto const insert_result = items_.insert(q, std::move(new_pile));
            BK_ASSERT(insert_result.second);
        } else {
            pile->add_item(std::move(i));
        }

        return result;
    }

    entity_instance_id add_object_at(unique_entity&& e, point2i32 const p) final override {
        auto const result = e.get();

        if (!entity_deleter_) {
            entity_deleter_ = &e.get_deleter();
        }

        BK_ASSERT(can_place_entity_at(p) == placement_result::ok);

        auto const q = underlying_cast_unsafe<int16_t>(p);

        auto const insert_result = entities_.insert(q, e.release());
        BK_ASSERT(insert_result.second);

        return result;
    }

    unique_entity remove_entity_at(point2i32 const p) noexcept final override {
        BK_ASSERT(!!entity_deleter_);
        auto const result = entities_.erase(underlying_cast_unsafe<int16_t>(p));
        return result.second
          ? unique_entity {result.first, *entity_deleter_}
          : unique_entity {entity_instance_id {}, *entity_deleter_};
    }

    unique_entity remove_entity(entity_instance_id const id) noexcept final override {
        return entities_.erase(id).second
          ? unique_entity {id, *entity_deleter_}
          : unique_entity {entity_instance_id {}, *entity_deleter_};
    }

    template <typename Predicate>
    std::pair<point2i32, placement_result> find_valid_placement_neareast_(
        random_state&   rng
      , point2i32 const p
      , int       const max_distance
      , Predicate       pred
    ) const noexcept  {
        auto const where = find_random_nearest(rng, p, max_distance, pred);
        if (!where.second) {
            return {p, placement_result::failed_obstacle};
        }

        return {where.first, placement_result::ok};
    }

    std::pair<point2i32, placement_result> find_valid_entity_placement_neareast(
        random_state&   rng
      , point2i32 const p
      , int       const max_distance
    ) const noexcept final override {
        return find_valid_placement_neareast_(rng, p, max_distance
          , [&](point2i32 const q) noexcept {
                return can_place_entity_at(q) == placement_result::ok;
            });
    }

    std::pair<point2i32, placement_result> find_valid_item_placement_neareast(
        random_state&   rng
      , point2i32 const p
      , int       const max_distance
    ) const noexcept final override {
        return find_valid_placement_neareast_(rng, p, max_distance
          , [&](point2i32 const q) noexcept {
                return can_place_item_at(q) == placement_result::ok;
            });
    }

    template <typename T>
    std::pair<point2i32, placement_result> add_object_nearest_random_(
        random_state&   rng
      , T&&             object
      , point2i32 const p
      , int       const max_distance
    ) {
        auto const where =
            find_valid_item_placement_neareast(rng, p, max_distance);

        if (where.second != placement_result::ok) {
            return where;
        }

        auto const result = add_object_at(std::move(object), where.first);

        return where;
    }

    std::pair<point2i32, placement_result> add_object_nearest_random(
        random_state&   rng
      , unique_item&&   i
      , point2i32 const p
      , int       const max_distance
    ) final override {
        return add_object_nearest_random_(rng, std::move(i), p, max_distance);
    }

    std::pair<point2i32, placement_result> add_object_nearest_random(
        random_state&   rng
      , unique_entity&& e
      , point2i32 const p
      , int       const max_distance
    ) final override {
        return add_object_nearest_random_(rng, std::move(e), p, max_distance);
    }

    size_t region_count() const noexcept final override {
        return regions_.size();
    }

    region_info region(size_t const i) const noexcept final override {
        BK_ASSERT(i < regions_.size());
        return regions_[i];
    }

    tile_view at(point2i32 const p) const noexcept final override;

    template <typename Container>
    auto make_range_(recti32 const area, Container const& c) const noexcept {
        auto const b = bounds();
        auto const r = clamp(area, b);

        return make_sub_region_range(as_const(c.data())
          , value_cast(r.x0),      value_cast(r.y0)
          , value_cast(b.width()), value_cast(b.height())
          , value_cast(r.width()), value_cast(r.height()));
    }

    const_sub_region_range<tile_id>
    tile_ids(recti32 const area) const noexcept final override {
        return make_range_(area, data_.ids);
    }

    const_sub_region_range<region_id>
    region_ids(recti32 const area) const noexcept final override {
        return make_range_(area, data_.region_ids);
    }

    merge_item_result impl_move_items_(
        point2i32 const from
      , item_pile& to
      , int const* const first
      , int const* const last
      , std::function<bool (item_instance_id)>          const& pred
      , std::function<void (unique_item&&, item_pile&)> const& sink
    ) {
        BK_ASSERT(( !first &&  !last)
               || (!!first && !!last));

        auto  const src_pos  = underlying_cast_unsafe<int16_t>(from);
        auto* const src_pile = items_.find(src_pos);
        if (!src_pile) {
            return merge_item_result::failed_bad_source;
        }

        auto const pile_sink = [&](unique_item&& itm) {
            sink(std::move(itm), to);
        };

        auto const size_before = src_pile->size();

        if (!first && !last) {
            src_pile->remove_if(pred, pile_sink);
        } else {
            src_pile->remove_if(first, last, pred, pile_sink);
        }

        auto const size_after = src_pile->size();

        BK_ASSERT(size_after <= size_before);

        if (src_pile->empty()) {
            items_.erase(src_pos);
            return merge_item_result::ok_merged_all;
        } else if (size_before - size_after == 0) {
            return merge_item_result::ok_merged_none;
        }

        return merge_item_result::ok_merged_some;
    }

    merge_item_result move_items(
        point2i32 from
      , item_pile& to
      , std::function<bool (item_instance_id)>          const& pred
      , std::function<void (unique_item&&, item_pile&)> const& sink
    ) final override {
        return impl_move_items_(from, to, nullptr, nullptr, pred, sink);
    }

    merge_item_result move_items(
        point2i32 from
      , item_pile& to
      , int const* first
      , int const* last
      , std::function<bool (item_instance_id)>          const& pred
      , std::function<void (unique_item&&, item_pile&)> const& sink
    ) final override {
        return impl_move_items_(from, to, first, last, pred, sink);
    }

    point2i32 stair_up(int const i) const noexcept final override {
        return stair_up_;
    }

    point2i32 stair_down(int const i) const noexcept final override {
        return stair_down_;
    }

    void for_each_pile(std::function<void (item_pile const&, point2i32)> const& f) final override {
        items_.for_each(f);
    }

    void for_each_entity(std::function<void (entity_instance_id, point2i32)> const& f) final override {
        entities_.for_each(f);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // implementation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    template <typename T>
    void copy_region(
        tile_data_set const*     const src
      , T const tile_data_set::* const src_field
      , recti32 const                  src_rect
      , std::vector<T>&                dst
    ) noexcept {
        auto const src_w = value_cast_unsafe<size_t>(src_rect.width());
        auto const dst_w = value_cast_unsafe<size_t>(width());

        BK_ASSERT(src_w <= dst_w);

        auto const step = dst_w - src_w;

        auto const x0 = value_cast(src_rect.x0);
        auto const x1 = value_cast(src_rect.x1);
        auto const y0 = value_cast(src_rect.y0);
        auto const y1 = value_cast(src_rect.y1);

        BK_ASSERT(x0 >= 0 && x1 >= 0 && y0 >= 0 && y1 >= 0);

        auto src_off = size_t {};
        auto dst_off = static_cast<size_t>(x0) + static_cast<size_t>(y0) * dst_w;

        for (auto y = y0; y < y1; ++y, dst_off += step) {
            for (auto x = x0; x < x1; ++x, ++src_off, ++dst_off) {
                dst[dst_off] = src[src_off].*src_field;
            }
        }
    }

    void place_doors(random_state& rng, recti32 area);

    void place_stairs(random_state& rng, recti32 area);

    void merge_walls_at(random_state& rng, recti32 area);

    void update_tile_ids(random_state& rng, recti32 area);

    void generate_make_connections(random_state& rng);

    void generate(random_state& rng);

    const_sub_region_range<tile_id> update_tile_rect(random_state& rng, recti32 const area, tile_data_set const* const data);

    const_sub_region_range<tile_id> update_tile_at(random_state& rng, point2i32 const p, tile_data_set const& data) noexcept final override {
        auto const r = recti32 {p, sizei32x {1}, sizei32y {1}};
        return update_tile_rect(rng, r, &data);
    }

    const_sub_region_range<tile_id> update_tile_rect(random_state& rng, recti32 const area, std::vector<tile_data_set> const& data) {
        return update_tile_rect(rng, area, data.data());
    }

    /////

    template <typename Predicate, typename UnaryF>
    point2i32 dig_segment(point2i32 p, region_id src_id, vec2i32 dir, int len
                        , Predicate pred, UnaryF on_connect) noexcept;

    // dig a one point of a path at 'p'; 'src_id' is the id of the region where
    // the path originated. The point 'p' must be in bounds and diggable.
    region_id dig_at(point2i32 p, region_id src_id) noexcept;

    // find, randomly, a good point to start or end a path within the area
    // given by 'bounds'.
    std::pair<point2i32, bool>
    find_path_end_point(random_state& rng
                      , recti32 region_bounds) const noexcept;

    template <typename UnaryF>
    void dig_random(random_state& rng, recti32 region_bounds
                  , UnaryF on_connect) noexcept;
private:
    template <typename Container>
    auto make_data_reader_(Container const& c) const noexcept {
        return make_at_xy_getter(c, width());
    }

    template <typename Container>
    auto make_data_reader_(Container& c) noexcept {
        return make_at_xy_getter(c, width());
    }

    auto make_bounds_checker_() const noexcept {
        return [&](auto const p) noexcept {
            return this->check_bounds_(p);
        };
    }

    template <typename Container, typename T>
    auto data_at_(Container&& c, point2<T> const p) noexcept
      -> decltype(c[0])
    {
        return at_xy(c, p, width());
    }

    template <typename Container, typename T>
    auto data_at_(Container&& c, point2<T> const p) const noexcept
      -> std::add_const_t<decltype(c[0])>
    {
        return const_cast<level_impl*>(this)->data_at_(c, p);
    }

    template <typename T>
    bool check_bounds_(point2<T> const p) const noexcept {
        return intersects(bounds(), p);
    }

    struct first_in_pile {
        item_instance_id operator()(item_pile const& p) const noexcept {
            return p.empty() ? item_instance_id {} : *p.begin();
        }
    };

    spatial_map<entity_instance_id, identity,      int16_t> entities_;
    spatial_map<item_pile,          first_in_pile, int16_t> items_;

    item_deleter   const* item_deleter_   {};
    entity_deleter const* entity_deleter_ {};

    recti32 bounds_;

    std::unique_ptr<bsp_generator> bsp_gen_;
    std::vector<region_info> regions_;

    point2i32 stair_up_   {0, 0};
    point2i32 stair_down_ {0, 0};

    struct data_t {
        explicit data_t(size_t const size)
          : ids(size)
          , types(size)
          , flags(size)
          , region_ids(size)
        {
        }

        data_t(sizei32x const width, sizei32y const height)
          : data_t {value_cast_unsafe<size_t>(width)
                  * value_cast_unsafe<size_t>(height)}
        {
        }

        std::vector<tile_id>    ids;
        std::vector<tile_type>  types;
        std::vector<tile_flags> flags;
        std::vector<region_id>  region_ids;
    } data_;

    world& world_;
    size_t id_;
//////

    template <typename UnaryF, typename Read, typename Check>
    point2i32 dig_path_segment_impl(
        point2i32 p
      , region_id const src_id
      , vec2i32 const dir
      , int const len
      , UnaryF on_connect
      , Read read
      , Check check
    ) {
        auto const must_stop = [&](point2i32 const p_cur, point2i32 const p_nxt, int const i) {
            // walls are a special case; look ahead by one tile to prevent
            // segments from stopping abruptly at a wall

            if (read(p_cur) != tile_type::wall) {
                return false;
            }

            auto const next_ok =
                check(p_nxt)
             && (can_gen_tunnel_at(p_nxt, read, check) == tribool::yes);

            auto const is_last = (i == len - 1);

            if (next_ok && is_last) {
                return data_at_(data_.flags, p_nxt).test(tile_flags::f_solid);
            } else if (next_ok && !is_last) {
                return false;
            }

            return true;
        };

        auto last_p = p;

        for (int i = 0; i < len; ++i) {
            // out of bounds
            if (!check(p)) {
                break;
            }

            auto const result = can_gen_tunnel_at(p, read, check);

            // hit a barrier
            if (result == tribool::no) {
                break;
            }

            auto const q = p + dir;

            if (must_stop(p, q, i)) {
                break;
            }

            // otherwise, just dig
            on_connect(dig_at(p, src_id));

            last_p = p;
            p = q;
        }

        return last_p;
    }

    template <typename UnaryF>
    point2i32 dig_path_segment(
        point2i32 const p
      , region_id const src_id
      , vec2i32 const dir
      , int const len
      , UnaryF on_connect
    ) {
        // segment end point; 1 extra for the lookahead
        auto const p_end = p + dir * (len + 1);

        auto const read = make_data_reader_(data_.types);

        auto const no_check = intersects(p,     shrink_rect(bounds(), 1))
                           && intersects(p_end, shrink_rect(bounds(), 2));

        return no_check
          // no need to check bounds
          ? dig_path_segment_impl(p, src_id, dir, len, on_connect, read, always_true {})
          // have to check bounds
          : dig_path_segment_impl(p, src_id, dir, len, on_connect, read, make_bounds_checker_());
    }

};

tile_view level_impl::at(point2i32 const p) const noexcept {
    if (!check_bounds_(p)) {
        static tile_id    const dummy_id        {};
        static tile_type  const dummy_type      {tile_type::empty};
        static tile_flags const dummy_flags     {};
        static region_id  const dummy_region_id {};
        static tile_data* const dummy_data      {};

        return {
            dummy_id
          , dummy_type
          , dummy_flags
          , dummy_region_id
          , dummy_data
        };
    }

    return {
        data_at_(data_.ids,        p)
      , data_at_(data_.types,      p)
      , data_at_(data_.flags,      p)
      , data_at_(data_.region_ids, p)
      , nullptr
    };

    ////////////////////


}

std::unique_ptr<level> make_level(
    random_state& rng
  , world& w
  , sizei32x const width
  , sizei32y const height
  , size_t const id
) {
    return std::make_unique<level_impl>(rng, w, width, height, id);
}

void level_impl::merge_walls_at(random_state& rng, recti32 const area) {
    auto const read = make_data_reader_(data_.types);
    transform_xy(area, bounds_, make_bounds_checker_()
      , [&](point2i32 const p, auto check) noexcept {
            // TODO: explicit 'this' due to a GCC bug (5.2.1)
            auto& type = this->data_at_(data_.types, p);
            if (type != tile_type::wall || !can_omit_wall_at(p, read, check)) {
                return;
            }

            type = tile_type::floor;
            this->data_at_(data_.flags, p) = tile_flags {0};
        }
    );
}

void level_impl::update_tile_ids(random_state& rng, recti32 const area) {
    auto const read    = make_data_reader_(data_.types);
    auto const read_id = make_data_reader_(data_.ids);

    transform_xy(area, bounds_, make_bounds_checker_()
      , [&](point2i32 const p, auto check) noexcept {
            // TODO: explicit 'this' due to a GCC bug (5.2.1)
            auto const id = tile_type_to_id_at(p, read, read_id, check);
            if (id != tile_id::invalid) {
                this->data_at_(data_.ids, p) = id;
            }
        }
    );
}

void level_impl::place_doors(random_state& rng, recti32 const area) {
    auto const read = make_data_reader_(data_.types);
    transform_xy(area, bounds_, make_bounds_checker_()
      , [&](point2i32 const p, auto check) noexcept {
            auto const id = try_place_door_at(p, read, check);
            if (id == tile_id::invalid || random_coin_flip(rng)) {
                return;
            }

            // TODO: explicit 'this' due to a GCC bug (5.2.1)
            this->data_at_(data_.types, p) = tile_type::door;
            this->data_at_(data_.ids, p) = id;
            this->data_at_(data_.flags, p) = tile_flags {tile_flags::f_solid};
        }
    );
}

void level_impl::place_stairs(random_state& rng, recti32 const area) {
    auto const get_random_region = [&]() noexcept {
        auto const is_candidate = [](region_info const& info) noexcept {
            return info.tile_count > 0;
        };

        auto const first_r = begin(regions_);
        auto const last_r  = end(regions_);

        // the number of candidate regions where a stair might be placed.
        auto const candidates = static_cast<int>(
            std::count_if(first_r, last_r, is_candidate));
        BK_ASSERT(candidates > 0);

        // choose a random candidate
        return [=, &rng]() noexcept {
            int const n = random_uniform_int(rng, 1, candidates);
            int       i = 0;

            auto const it = std::find_if(first_r, last_r
              , [&](region_info const& info) noexcept {
                    return is_candidate(info) && (++i >= n);
                });

            BK_ASSERT(it != last_r);

            return *it;
        };
    }();

    // find a random valid position within the chosen candidate
    auto const find_stair_pos = [&](recti32 const r) noexcept {
        auto const is_ok = [&](point2i32 const p) noexcept {
            return data_at_(data_.types, p) == tile_type::floor;
        };

        for (int i = 0; i < 1000; ++i) {
            auto const p = random_point_in_rect(rng, r);
            if (is_ok(p)) {
                return p;
            }
        }

        return r.top_left();
    };

    auto const make_stair_at = [&](point2i32 const p, tile_id const id) noexcept {
        data_at_(data_.types, p) = tile_type::stair;
        data_at_(data_.ids, p)   = id;
        data_at_(data_.flags, p) = tile_flags {};
        return p;
    };

    stair_up_ = make_stair_at(find_stair_pos(get_random_region().bounds)
                            , tile_id::stair_up);

    stair_down_ = make_stair_at(find_stair_pos(get_random_region().bounds)
                              , tile_id::stair_down);
}

region_id level_impl::dig_at(
    point2i32 const p
  , region_id const src_id
) noexcept {
    auto& to_type  = data_at_(data_.types,      p);
    auto& to_flags = data_at_(data_.flags,      p);
    auto& to_id    = data_at_(data_.region_ids, p);

    auto const result = (to_type != tile_type::empty)
      ? to_id
      : src_id;

    if (to_type == tile_type::empty) {
        to_type = tile_type::tunnel;
        to_flags.clear(tile_flags::f_solid);
    } else if (to_type == tile_type::wall) {
        to_type = tile_type::floor;
        to_flags.clear(tile_flags::f_solid);
    }

    to_id = src_id;

    return result;
}

template <typename Predicate, typename UnaryF>
point2i32 level_impl::dig_segment(
    point2i32       p
  , region_id const src_id
  , vec2i32   const dir
  , int       const len
  , Predicate       pred
  , UnaryF          on_connect
) noexcept {
    BK_ASSERT(is_cardinal_dir(dir) && len > 0 && intersects(bounds(), p));

    int n = 0;
    for (int i = 0; i < len; ++i) {
        auto const q = p + dir;
        if (!pred(q, n)) {
            break;
        }

        p = q;

        auto const id = dig_at(p, src_id);
        if (on_connect(id)) {
            ++n;
        }
    }

    return p;
}

std::pair<point2i32, bool>
level_impl::find_path_end_point(
    random_state& rng
  , recti32 const region_bounds
) const noexcept {
    auto const is_valid_start_point = [&](point2i32 const p) noexcept {
        return data_at_(data_.types, p) == tile_type::floor;
    };

    return find_if_random(rng, region_bounds
      , [&](point2i32 const p) noexcept {
            return data_at_(data_.types, p) == tile_type::floor;
        });
}

template <typename UnaryF>
void level_impl::dig_random(
    random_state& rng
  , recti32 const region_bounds
  , UnaryF        on_connect
) noexcept {
    auto const end_point_pair = find_path_end_point(rng, region_bounds);
    if (!end_point_pair.second) {
        BK_ASSERT(false); // TODO
        return;
    }

    auto       p        = end_point_pair.first;
    auto const src_id   = data_at_(data_.region_ids, p);
    auto const segments = random_uniform_int(rng, 1, 10);

    for (int s = 0; s < segments; ++s) {
        auto const dir = random_dir4(rng);
        auto const len = random_uniform_int(rng, 3, 10);

        p = dig_path_segment(p, src_id, dir, len, on_connect);
    }
}

void level_impl::generate_make_connections(random_state& rng) {
    auto const region_count = static_cast<int>(regions_.size());

    using vertex_t     = int16_t;
    using graph_data_t = int8_t;

    auto graph      = adjacency_matrix<vertex_t> {region_count};
    auto graph_data = vertex_data<graph_data_t>  {region_count};

    auto component_sizes    = std::vector<vertex_t> {};
    auto component_indicies = std::vector<vertex_t> {};

    component_sizes.reserve(region_count);
    component_indicies.reserve(region_count);

    // fill 'component_indicies', in random order, with the indicies of the
    // components which have 'n' components; the first such index is at 'first_i'.
    auto const get_component_indicies = [&](size_t const first_i, vertex_t const n) noexcept {
        component_indicies.clear();

        auto const first = next(
            begin(component_sizes), static_cast<ptrdiff_t>(first_i));
        auto const last = end(component_sizes);

        auto const i = static_cast<vertex_t>(first_i);

        fill_with_index_if(first, last, i, back_inserter(component_indicies)
          , [n](vertex_t const m) noexcept { return m == n; });

        shuffle(begin(component_indicies), end(component_indicies), rng);
    };

    // return 'to' if an edge between from<->to is added to graph, otherwise
    // return 'from'.
    auto const add_connection = [&](region_id const from, region_id const to) noexcept {
        BK_ASSERT(value_cast(to)   > 0
               && value_cast(from) > 0);

        // no self connections
        if (from == to) {
            return from;
        }

        // valid region ids are >= 1; correct for this fact
        auto const v0 = value_cast(from) - 1;
        auto const v1 = value_cast(to)   - 1;

        // already connected
        if (graph(v0, v1)) {
            BK_ASSERT(graph(v1, v0)); // the connection should be reciprocal
            return from;
        }

        graph.add_mutual_edge(v0, v1);
        return to;
    };

    auto const read_type = make_data_reader_(data_.types);

    auto const find_nth_random = [&](auto&& c, auto const n, auto const& value) {
        using std::begin;
        using std::end;

        auto const first = begin(c);
        auto const last  = end(c);

        auto const which =
            random_uniform_int(rng, 0, static_cast<int32_t>(n - 1));

        return std::distance(first, find_nth(first, last, which, value));
    };

    connect_components(graph, graph_data, [&](int const n) {
        BK_ASSERT(n <= region_count && n > 1);

        size_t   min_component_i = 0;
        vertex_t min_component_n = 0;

        std::tie(min_component_i, std::ignore, min_component_n, std::ignore) =
            count_components(graph_data, component_sizes);

        BK_ASSERT(min_component_n > 0);

        get_component_indicies(min_component_i, min_component_n);
        BK_ASSERT(component_indicies.size() > 0);

        for (vertex_t const i : component_indicies) {
           // components are 1-based
           auto const c     = static_cast<vertex_t>(i + 1);
           auto const off   = find_nth_random(graph_data, min_component_n, c);
           auto const index = static_cast<size_t>(off);

           region_id const src_id = static_cast<region_id::type>(c);

           dig_random(rng, region(index).bounds
             , [&](region_id const id) noexcept {
                   add_connection(src_id, id);
               }
            );
        }

        return true;
    });
}

void level_impl::generate(random_state& rng) {
    fill(data_.ids,        tile_id::invalid);
    fill(data_.region_ids, region_id {});
    fill(data_.types,      tile_type::empty);
    fill(data_.flags,      tile_flags {tile_flags::f_solid});

    auto&       bsp = *bsp_gen_;
    auto const& p   = bsp.params();

    // generate a bsp-based layout, populate regions_ with the result, and
    // return the min and max region areas generated.
    auto const generate_regions = [&] {
        bsp.clear();
        bsp.generate(rng);

        regions_.clear();
        regions_.reserve(bsp.size());

        auto max_area = std::numeric_limits<int32_t>::min();
        auto min_area = std::numeric_limits<int32_t>::max();

        for (auto const& node : bsp) {
            auto const area = value_cast(node.rect.area());
            BK_ASSERT(area >= 0);

            max_area = std::max(max_area, area);
            min_area = std::min(min_area, area);

            regions_.push_back({node.rect, 0, 0, 0, 0});
        }

        BK_ASSERT(max_area >= min_area
               && min_area >= 0);

        return std::make_pair(min_area, max_area);
    };

    // generate a random room-sized rect
    auto const generate_rect =
        generate_rect_room {p.min_room_size, p.max_room_size};

    // roll whether to generate a room or not
    auto const roll_room_chance = [
        &rng
      , lo = value_cast(p.room_chance_num)
      , hi = value_cast(p.room_chance_den)
    ]() noexcept {
        return random_chance_in_x(rng, lo, hi);
    };

    // min and max region sizes
    auto const region_area_range = generate_regions();

    // a buffer to use for room generation
    std::vector<tile_data_set> buffer;
    buffer.reserve(static_cast<size_t>(region_area_range.second));

    tile_data_set default_tile {
        tile_data  {}
      , tile_flags {tile_flags::f_solid}
      , tile_id    {}
      , tile_type  {tile_type::empty}
      , region_id  {}
    };

    // region id for the next room generated; 0 is for unused regions only.
    auto next_rid = value_cast(default_tile.rid);

    // for each region generated, generate a room (or not)
    for (auto& region : regions_) {
        if (!roll_room_chance()) {
            continue;
        }

        // ok, generate a room with the next id.
        default_tile.rid = ++next_rid;
        region.id = next_rid;

        auto const rect = region.bounds;

        // resize the buffer to match the size of the region and fill it with
        // the default tile values
        buffer.resize(value_cast_unsafe<size_t>(rect.area()), default_tile);

        // generate a random sized room
        region.tile_count = generate_rect(rng, rect, buffer);

        copy_region(buffer.data(), &tile_data_set::rid,   rect, data_.region_ids);
        copy_region(buffer.data(), &tile_data_set::id,    rect, data_.ids);
        copy_region(buffer.data(), &tile_data_set::type,  rect, data_.types);
        copy_region(buffer.data(), &tile_data_set::flags, rect, data_.flags);

        buffer.clear();
    }

    // remove unused regions
    {
        auto const first = begin(regions_);
        auto const last  = end(regions_);
        auto const it    = std::remove_if(first, last
          , [](auto const& info) noexcept { return info.tile_count <= 0; });
        regions_.erase(it, last);
    }

    merge_walls_at(rng, bounds_);

    // do a first pass so that natural wall ids are chosen
    update_tile_ids(rng, bounds_);

    place_stairs(rng, bounds_);
    generate_make_connections(rng);
    place_doors(rng, bounds_);

    // do a final pass to update anything changed by corridors, etc.
    update_tile_ids(rng, bounds_);
}

const_sub_region_range<tile_id>
level_impl::update_tile_rect(
    random_state&              rng
  , recti32              const area
  , tile_data_set const* const data
) {
    copy_region(data, &tile_data_set::id,    area, data_.ids);
    copy_region(data, &tile_data_set::type,  area, data_.types);
    copy_region(data, &tile_data_set::flags, area, data_.flags);

    auto update_area = grow_rect(area);
    update_area.x0 = std::max(update_area.x0, bounds_.x0);
    update_area.y0 = std::max(update_area.y0, bounds_.y0);
    update_area.x1 = std::min(update_area.x1, bounds_.x1);
    update_area.y1 = std::min(update_area.y1, bounds_.y1);

    update_tile_ids(rng, update_area);

    return make_sub_region_range(as_const(data_.ids.data())
      , value_cast(update_area.x0),      value_cast(update_area.y0)
      , value_cast(bounds_.width()),     value_cast(bounds_.height())
      , value_cast(update_area.width()), value_cast(update_area.height()));
}

} //namespace boken
