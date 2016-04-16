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

template <typename T, typename Read, typename Check>
bool can_gen_tunnel_at_wall(point2<T> const p, Read read, Check check) noexcept {
    auto const is_wall = [&](point2<T> const q) noexcept {
        return read(q) == tile_type::wall;
    };

    auto const is_not_wall = [&](point2<T> const q) noexcept {
        return read(q) != tile_type::wall;
    };

    auto const wall_type  = fold_neighbors4(p, check, is_wall);
    auto const other_type = fold_neighbors4(p, check, is_not_wall);

    return (wall_type == 0b1001 && other_type == 0b0110)
        || (wall_type == 0b0110 && other_type == 0b1001);
}

template <typename T, typename Read, typename Check>
bool can_gen_tunnel_at(point2<T> const p, Read read, Check check) noexcept {
    if (!check(p)) {
        return false;
    }

    switch (read(p)) {
    case tile_type::empty  :
    case tile_type::floor  :
    case tile_type::tunnel :
    case tile_type::door   :
    case tile_type::stair  :
        return true;
    case tile_type::wall :
        return can_gen_tunnel_at_wall(p, read, check);
    default :
        break;
    }

    return false;
}

} // namespace boken

namespace {

template <typename Read, typename Check>
boken::tile_id tile_type_to_id_at(boken::point2i32 const p, Read read, Check check) noexcept {
    using namespace boken;

    static_assert(noexcept(read(p)), "");
    static_assert(noexcept(check(p)), "");

    using ti = tile_id;
    using tt = tile_type;

    switch (read(p)) {
    default         : break;
    case tt::empty  : return ti::empty;
    case tt::floor  : return ti::floor;
    case tt::tunnel : return ti::tunnel;
    case tt::door   : break;
    case tt::stair  : break;
    case tt::wall   : return wall_type_from_neighbors(
        fold_neighbors4(p, check, [&](point2i32 const q) noexcept {
            auto const type = read(q);
            return type == tt::wall || type == tt::door;
        }));
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

    int32_t operator()(random_state& rng, recti32 const area, std::vector<tile_data_set>& out) {
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
      : entities_ {value_cast_unsafe<int16_t>(width), value_cast_unsafe<int16_t>(height), get_entity_instance_id_t {}, get_entity_id_t {w}}
      , items_    {value_cast_unsafe<int16_t>(width), value_cast_unsafe<int16_t>(height), get_item_instance_id_t {}, get_item_id_t {w}}
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
        return bsp_gen_->size();
    }

    region_info region(size_t const i) const noexcept final override {
        BK_ASSERT(i < regions_.size());
        return regions_[i];
    }

    tile_view at(point2i32 const p) const noexcept final override;

    std::pair<point2i16 const*, point2i16 const*>
    entity_positions() const noexcept final override {
        return entities_.positions_range();
    }

    std::pair<entity_id const*, entity_id const*>
    entity_ids() const noexcept final override {
        return entities_.properties_range();
    }

    std::pair<point2i16 const*, point2i16 const*>
    item_positions() const noexcept final override {
        return items_.positions_range();
    }

    std::pair<item_id const*, item_id const*>
    item_ids() const noexcept final override {
        return items_.properties_range();
    }

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

    struct get_entity_instance_id_t {
        entity_instance_id operator()(entity_instance_id const id) const noexcept {
            return id;
        }
    };

    struct get_entity_id_t {
        get_entity_id_t(world const& w) noexcept : world_ {w} {}

        entity_id operator()(entity_instance_id const id) const noexcept {
            return get_id(boken::find(world_, id));
        }

        world const& world_;
    };

    struct get_item_instance_id_t {
        item_instance_id operator()(item_pile const& p) const noexcept {
            return p.empty() ? item_instance_id {} : *p.begin();
        }
    };

    struct get_item_id_t {
        get_item_id_t(world const& w) noexcept : world_ {w} {}

        item_id operator()(item_pile const& p) const noexcept {
            return p.empty()
              ? item_id {}
              : get_id(boken::find(world_, *p.begin()));
        }

        world const& world_;
    };

    spatial_map<entity_instance_id, get_entity_instance_id_t, get_entity_id_t, int16_t> entities_;
    spatial_map<item_pile, get_item_instance_id_t, get_item_id_t, int16_t> items_;

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
          : data_t {value_cast_unsafe<size_t>(width) * value_cast_unsafe<size_t>(height)}
        {
        }

        std::vector<tile_id>    ids;
        std::vector<tile_type>  types;
        std::vector<tile_flags> flags;
        std::vector<region_id>  region_ids;
    } data_;

    world& world_;
    size_t id_;
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
    auto const read = make_data_reader_(data_.types);
    transform_xy(area, bounds_, make_bounds_checker_()
      , [&](point2i32 const p, auto check) noexcept {
            // TODO: explicit 'this' due to a GCC bug (5.2.1)
            auto const id = tile_type_to_id_at(p, read, check);
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

void level_impl::generate_make_connections(random_state& rng) {
    auto const region_has_room = [](region_info const& info) noexcept {
        return info.tile_count > 0;
    };

    auto const find_path_start = [&](region_info const& info) noexcept {
        auto const is_valid_start_point = [&](point2i32 const p) noexcept {
            return data_at_(data_.types, p) == tile_type::floor;
        };

        auto const p = find_if_random(rng, info.bounds, is_valid_start_point);
        BK_ASSERT(p.second);
        return p.first;
    };

    auto const level_bounds = bounds();
    auto       read         = make_data_reader_(data_.types);
    auto       check        = make_bounds_checker_();

    for (auto const& region : regions_) {
        if (!region_has_room(region)) {
            continue;
        }

        bool const must_check = intersects_edge(region.bounds, level_bounds);

        auto p = find_path_start(region);
        auto const segments = random_uniform_int(rng, 0, 10);

        for (int s = 0; s < segments; ++s) {
            auto const dir = random_dir4(rng);
            auto const len = random_uniform_int(rng, 3, 10);

            for (int i = 0; i < len; ++i) {
                auto const p0 = p + dir;
                auto const ok = must_check
                  ? can_gen_tunnel_at(p0, read, check)
                  : can_gen_tunnel_at(p0, read, always_true {});

                if (!ok) {
                    break;
                }

                p = p0;

                auto& type = data_at_(data_.types, p);
                if (type == tile_type::empty) {
                    type = tile_type::tunnel;
                    data_at_(data_.flags, p) = tile_flags {0};
                } else if (type == tile_type::wall) {
                    type = tile_type::floor;
                    data_at_(data_.flags, p) = tile_flags {0};
                }
            }
        }
    }
}

void level_impl::generate(random_state& rng) {
    std::fill(begin(data_.types), end(data_.types), tile_type::empty);
    std::fill(begin(data_.flags), end(data_.flags), tile_flags {tile_flags::f_solid});

    auto& bsp = *bsp_gen_;

    auto const& p = bsp.params();

    bsp.clear();
    bsp.generate(rng);

    regions_.clear();
    regions_.reserve(bsp.size());
    std::transform(std::begin(bsp), std::end(bsp), back_inserter(regions_)
      , [](auto const& node) noexcept {
          return region_info {node.rect, 0, 0, 0};
      });

    // reserve enough space for the largest region
    std::vector<tile_data_set> buffer;
    {
        auto const size = std::max_element(std::begin(bsp), std::end(bsp)
          , [](auto const& node_a, auto const& node_b) noexcept {
              return node_a.rect.area() < node_b.rect.area();
          })->rect.area();

        buffer.reserve(static_cast<size_t>(std::max(0, value_cast(size))));
    }

    auto next_rid = region_id::type {0};

    tile_data_set default_tile {
        tile_data  {}
      , tile_flags {tile_flags::f_solid}
      , tile_id    {}
      , tile_type  {tile_type::empty}
      , next_rid
    };

    auto gen_rect = generate_rect_room {p.min_room_size, p.max_room_size};

    auto const roll_room_chance = [&]() noexcept {
        return random_chance_in_x(rng, value_cast(p.room_chance_num)
                                     , value_cast(p.room_chance_den));
    };

    for (auto& region : regions_) {
        auto const rect = region.bounds;
        fill_rect(data_.region_ids, width(), rect, (default_tile.rid = next_rid++));

        if (!roll_room_chance()) {
            continue;
        }

        buffer.resize(static_cast<size_t>(std::max(0, value_cast(rect.area()))), default_tile);
        region.tile_count = gen_rect(rng, rect, buffer);

        copy_region(buffer.data(), &tile_data_set::id,    rect, data_.ids);
        copy_region(buffer.data(), &tile_data_set::type,  rect, data_.types);
        copy_region(buffer.data(), &tile_data_set::flags, rect, data_.flags);

        buffer.resize(0);
    }

    merge_walls_at(rng, bounds_);
    generate_make_connections(rng);
    place_stairs(rng, bounds_);
    place_doors(rng, bounds_);
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
