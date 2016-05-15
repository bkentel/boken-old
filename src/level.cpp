#include "level.hpp"
#include "level_details.hpp"

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

namespace boken {

namespace detail {

string_view impl_can_add_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
  , const_item_descriptor   const itm
  , const_level_location    const dest
) noexcept {
    return {};
}

string_view impl_can_remove_item(
    const_context           const ctx
  , const_entity_descriptor const subject
  , point2i32               const subject_p
  , const_item_descriptor   const itm
  , const_level_location    const dest
) noexcept {
    return {};
}

} // namespace detail

void merge_into_pile(
    context         const ctx
  , unique_item           itm_ptr
  , item_descriptor const itm
  , level_location  const pile
) {
    pile.lvl.add_object_at(std::move(itm_ptr), pile.p);
}

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

bool can_gen_tunnel_at_wall(uint32_t const neighbors) noexcept {
    //                     NWES
    return (neighbors == 0b1001u)
        || (neighbors == 0b0110u);
}

bool is_door_candidate(tile_type const type) noexcept {
    using tt = tile_type;

    switch (type) {
    case tt::floor:  BK_ATTRIBUTE_FALLTHROUGH;
    case tt::tunnel: BK_ATTRIBUTE_FALLTHROUGH;
    case tt::wall:   return true;
    case tt::stair:  BK_ATTRIBUTE_FALLTHROUGH;
    case tt::empty:  BK_ATTRIBUTE_FALLTHROUGH;
    case tt::door:   break;
    default:         BK_ASSERT(false); break;
    }

    return false;
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
                data.flags = tile_flag::solid;
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

//! level tile data blob
struct level_data_t {
    explicit level_data_t(size_t const size)
      : ids(size, tile_id::invalid)
      , types(size, tile_type::empty)
      , flags(size, tile_flag::solid)
      , region_ids(size, region_id {})
    {
    }

    static size_t get_size_(sizei32x const w, sizei32y const h) noexcept {
        auto const sx = value_cast(w);
        auto const sy = value_cast(h);

        BK_ASSERT(sx > 0 && sy > 0);

        return static_cast<size_t>(sx * sy);
    }

    level_data_t(sizei32x const width, sizei32y const height)
      : level_data_t {get_size_(width, height)}
    {
    }

    std::vector<tile_id>    ids;
    std::vector<tile_type>  types;
    std::vector<tile_flags> flags;
    std::vector<region_id>  region_ids;
};

class level_impl;

//! adapt level's interface to what the a_star_pather expects
class level_adapter {
public:
    using point = point2i32;

    level_adapter(level_impl const& lvl) noexcept : lvl_ {lvl} {}

    bool is_passable(point p) const noexcept;

    bool is_in_bounds(point p) const noexcept;

    int32_t cost(point from, point to) const noexcept;

    template <typename Predicate, typename UnaryF>
    void for_each_neighbor_if(point p, Predicate pred, UnaryF f) const noexcept;

    template <typename UnaryF>
    void for_each_neighbor(point const p, UnaryF f) const noexcept {
        for_each_neighbor_if(p, [](auto&&) noexcept { return true; }, f);
    }

    int32_t width()  const noexcept;
    int32_t height() const noexcept;
    int32_t size()   const noexcept;
private:
    level_impl const& lvl_;
};

class level_impl : public level {
    friend level_adapter; // TODO consider add accessor functions instead
public:
    level_impl(random_state& rng, world& w, sizei32x width, sizei32y height
             , size_t id);

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

    std::pair<bool, point2i32> find_position(
        entity_instance_id const id
    ) const noexcept final override {
        auto const result = entities_.find(id);
        if (!result.first) {
            return {false, {}};
        }

        return {true, result.second};
    }

    std::pair<entity*, point2i32>
    find(entity_instance_id const id) noexcept final override {
        auto const p = find_position(id);
        if (!p.first) {
            return {nullptr, {}};
        }

        return {&boken::find(world_, id), p.second};
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
             : data_at_(data_.flags, p).test(tile_flag::solid)
                 ? placement_result::failed_obstacle
             : is_entity_at(p)
                 ? placement_result::failed_entity
                 : placement_result::ok;
    }

    placement_result can_place_item_at(point2i32 const p) const noexcept final override {
        return !check_bounds_(p)
                 ? placement_result::failed_bounds
             : data_at_(data_.flags, p).test(tile_flag::solid)
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
        transform_f          transform
      , transform_callback_f callback
    ) final override {
        auto const values    = entities_.values_range();
        auto const positions = entities_.positions_range();

        auto v_it = values.first;
        auto p_it = positions.first;

        for (size_t i = 0; i < entities_.size(); ++i, ++v_it, ++p_it) {
            auto const p  = *p_it;
            auto const id = *v_it;

            auto const result = transform(id, p);

            auto const q = std::get<1>(result);
            if (p == q) {
                continue;
            }

            callback(std::get<0>(result), move_by(id, q - p), p, q);
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

        if (where.second == placement_result::ok) {
            add_object_at(std::move(object), where.first);
        }

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

    std::pair<merge_item_result, int> impl_move_items_(
        point2i32 const from
      , int const* const first
      , int const* const last
      , std::function<void (unique_item&&)> const& pred
    ) {
        BK_ASSERT(( !first &&  !last)
               || (!!first && !!last));

        auto  const src_pos  = underlying_cast_unsafe<int16_t>(from);
        auto* const src_pile = items_.find(src_pos);
        if (!src_pile) {
            return {merge_item_result::failed_bad_source, 0};
        }

        auto const trans = [&](int const i) noexcept {
            BK_ASSERT(i >= 0);
            return (*src_pile)[static_cast<size_t>(i)];
        };

        auto const n = (!first && !last)
          ? src_pile->remove_if(pred)
          : src_pile->remove_if(first, last, trans, pred);

        if (src_pile->empty()) {
            items_.erase(src_pos);
            return {merge_item_result::ok_merged_all, n};
        } else if (n == 0) {
            return {merge_item_result::ok_merged_none, 0};
        }

        return {merge_item_result::ok_merged_some, n};
    }

    std::pair<merge_item_result, int> move_items(
        point2i32 const from
      , std::function<void (unique_item&&)> const& pred
    ) final override {
        return impl_move_items_(from, nullptr, nullptr, pred);
    }

    std::pair<merge_item_result, int> move_items(
        point2i32 const from
      , int const* const first
      , int const* const last
      , std::function<void (unique_item&&)> const& pred
    ) final override {
        return impl_move_items_(from, first, last, pred);
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

    std::vector<point2i32> const&
    find_path(point2i32 const from, point2i32 const to) const final override {
        BK_ASSERT(check_bounds_(from)
               && check_bounds_(to));

        last_path.clear();

        pather.search({*this}, from, to, diagonal_heuristic());
        pather.reverse_copy_path(from, to, back_inserter(last_path));
        std::reverse(begin(last_path), end(last_path));

        return last_path;
    }

    const_sub_region_range<tile_id>
    update_tile_at(random_state& rng, point2i32 p
                 , tile_data_set const& data) noexcept final override;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // implementation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    template <typename T>
    void copy_region(tile_data_set const* src
                   , T const tile_data_set::* src_field, recti32 src_rect
                   , std::vector<T>& dst) noexcept;

    void place_doors(random_state& rng, recti32 area);

    void place_stairs(random_state& rng, recti32 area);

    void merge_walls_at(random_state& rng, recti32 area);

    void update_tile_ids(random_state& rng, recti32 area);

    void generate_make_connections(random_state& rng);

    void generate(random_state& rng);

    const_sub_region_range<tile_id>
    update_tile_rect(random_state& rng, recti32 area
                   , tile_data_set const* data);

    const_sub_region_range<tile_id>
    update_tile_rect(random_state& rng, recti32 const area
                   , std::vector<tile_data_set> const& data
    ) {
        return update_tile_rect(rng, area, data.data());
    }

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

    template <typename UnaryF, typename Read, typename Check>
    point2i32 dig_path_segment_impl(point2i32 p, region_id src_id, vec2i32 dir
                                  , int len, UnaryF on_connect, Read read
                                  , Check check);

    template <typename UnaryF>
    point2i32 dig_path_segment(point2i32 p, region_id src_id, vec2i32 dir
                             , int len, UnaryF on_connect);

    auto make_bounds_checker_() const noexcept {
        return make_bounds_checker(bounds());
    }

    template <typename T>
    bool check_bounds_(point2<T> const p) const noexcept {
        return intersects(bounds(), p);
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

    struct first_in_pile {
        item_instance_id operator()(item_pile const& p) const noexcept {
            return p.empty() ? item_instance_id {} : *p.begin();
        }
    };
private:
    spatial_map<entity_instance_id, identity,      int16_t> entities_;
    spatial_map<item_pile,          first_in_pile, int16_t> items_;

    item_deleter   const* item_deleter_   {};
    entity_deleter const* entity_deleter_ {};

    recti32 bounds_;

    std::unique_ptr<bsp_generator> bsp_gen_;
    std::vector<region_info> regions_;

    point2i32 stair_up_   {0, 0};
    point2i32 stair_down_ {0, 0};

    level_data_t data_;

    world& world_;
    size_t id_;

    // logically const, but keeps a mutable buffer internally used across
    // invocations
    a_star_pather<level_adapter> mutable pather;
    std::vector<point2i32> mutable last_path;
private:
    template <typename T>
    class data_read_write_base {
    public:
        data_read_write_base(T* const data, sizei32x const w) noexcept
          : data_ {data}, w_ {w}
        {
        }

        tile_type tile_type_at(point2i32 const p) const noexcept {
            return at_xy(data_->types, p, w_);
        }

        tile_id tile_id_at(point2i32 const p) const noexcept {
            return at_xy(data_->ids, p, w_);
        }
    protected:
        T*       data_;
        sizei32x w_;
    };

    struct data_reader : public data_read_write_base<level_data_t const> {
        using data_read_write_base::data_read_write_base;
    };

    struct data_writer : public data_read_write_base<level_data_t> {
        using data_read_write_base::data_read_write_base;

        void set_tile_type_at(point2i32 const p, tile_type const type) noexcept {
            at_xy(data_->types, p, w_) = type;
        }

        void set_tile_id_at(point2i32 const p, tile_id const id) noexcept {
            at_xy(data_->ids, p, w_) = id;
        }

        void set_tile_flags_at(point2i32 const p, tile_flags const flags) noexcept {
            at_xy(data_->flags, p, w_) = flags;
        }
    };

    data_reader make_data_reader() const noexcept {
        return {&data_, bounds_.width()};
    }

    data_writer make_data_writer() noexcept {
        return {&data_, bounds_.width()};
    }
};

std::unique_ptr<level> make_level(
    random_state&  rng
  , world&         w
  , sizei32x const width
  , sizei32y const height
  , size_t   const id
) {
    return std::make_unique<level_impl>(rng, w, width, height, id);
}

//===------------------------------------------------------------------------===
// level_adapter
//===------------------------------------------------------------------------===

bool level_adapter::is_passable(point const p) const noexcept {
    return !lvl_.data_at_(lvl_.data_.flags, p).test(tile_flag::solid);
}

bool level_adapter::is_in_bounds(point const p) const noexcept {
    return lvl_.check_bounds_(p);
}

int32_t level_adapter::cost(
    point const // from
  , point const // to
) const noexcept {
    return 1;
}

template <typename Predicate, typename UnaryF>
void level_adapter::for_each_neighbor_if(
    point const p
  , Predicate pred
  , UnaryF f
) const noexcept {
    using v = vec2<int>;
    constexpr std::array<v, 8> dir {
        v {-1, -1}, v { 0, -1}, v { 1, -1}
      , v {-1,  0},             v { 1,  0}
      , v {-1,  1}, v { 0,  1}, v { 1,  1}
    };

    for (auto const& d : dir) {
        point const p0 = p + d;
        if (is_in_bounds(p0) && pred(p0) && is_passable(p0)) {
            f(p0);
        }
    }
}

int32_t level_adapter::width()  const noexcept { return value_cast(lvl_.width()); }
int32_t level_adapter::height() const noexcept { return value_cast(lvl_.height()); }
int32_t level_adapter::size()   const noexcept { return width() * height(); }

//===------------------------------------------------------------------------===

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

level_impl::level_impl(random_state& rng, world& w, sizei32x const width, sizei32y const height, size_t const id)
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

void level_impl::merge_walls_at(random_state& rng, recti32 const area) {
    auto data = make_data_writer();

    transform_xy(area, bounds_, make_bounds_checker_()
      , [&](point2i32 const p, auto check) noexcept {
            auto const type = data.tile_type_at(p);
            if (type != tile_type::wall || !can_omit_wall_at(p, data, check)) {
                return;
            }

            data.set_tile_type_at(p, tile_type::floor);
            data.set_tile_flags_at(p, tile_flags {0});
        }
    );
}

void level_impl::update_tile_ids(random_state& rng, recti32 const area) {
    auto data = make_data_writer();

    transform_xy(area, bounds_, make_bounds_checker_()
      , [&](point2i32 const p, auto check) noexcept {
            auto const id = get_id_at(p, data, check);
            if (id == tile_id::invalid) {
                return;
            }

            data.set_tile_id_at(p, id);
        }
    );
}

void level_impl::place_doors(random_state& rng, recti32 const area) {
    auto data = make_data_writer();

    transform_xy(area, bounds_, make_bounds_checker_()
      , [&](point2i32 const p, auto check) noexcept {
            auto const id = try_place_door_at(p, data, check);
            auto const ok = (id != tile_id::invalid)
                         && random_coin_flip(rng);

            if (!ok) {
                return;
            }

            data.set_tile_type_at(p, tile_type::door);
            data.set_tile_id_at(p, id);
            data.set_tile_flags_at(p, tile_flag::solid);
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

    if (to_type == tile_type::empty) {
        to_type = tile_type::tunnel;
        to_flags.clear(tile_flag::solid);
        to_id = src_id;
        return src_id;
    } else if (to_type == tile_type::wall) {
        to_type = tile_type::floor;
        to_flags.clear(tile_flag::solid);
        to_id = src_id;
    }

    return to_id;
}

template <typename UnaryF, typename Read, typename Check>
point2i32 level_impl::dig_path_segment_impl(
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

        if (read.tile_type_at(p_cur) != tile_type::wall) {
            return false;
        }

        auto const next_ok =
            check(p_nxt)
         && can_gen_tunnel_at(p_nxt, read, check);

        auto const is_last = (i == len - 1);

        if (next_ok && is_last) {
            return data_at_(data_.flags, p_nxt).test(tile_flag::solid);
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

        // hit a barrier
        if (!can_gen_tunnel_at(p, read, check)) {
            break;
        }

        auto const q = p + dir;
        if (must_stop(p, q, i)) {
            break;
        }

        // otherwise, just dig
        auto const dig_id = dig_at(p, src_id);
        if (dig_id != src_id) {
            on_connect(dig_id, p);
        }

        for_each_xy_edge(grow_rect(recti32 {p, p}), [&](point2i32 const p0) {
            // out of bounds
            if (!check(p0)) {
                return;
            }

            // solid
            if (data_at_(data_.flags, p0).test(tile_flag::solid)) {
                return;
            }

            auto const id = data_at_(data_.region_ids, p0);
            if (id == region_id {} || id == src_id) {
                return;
            }

            on_connect(id, p0);
        });

        last_p = p;
        p = q;
    }

    return last_p;
}

template <typename UnaryF>
point2i32 level_impl::dig_path_segment(
    point2i32 const p
  , region_id const src_id
  , vec2i32 const dir
  , int const len
  , UnaryF on_connect
) {
    // segment end point; 1 extra for the lookahead
    auto const p_end = p + dir * (len + 1);

    auto const read = make_data_reader();

    auto const no_check = intersects(p,     shrink_rect(bounds(), 1))
                       && intersects(p_end, shrink_rect(bounds(), 2));

    return no_check
        // no need to check bounds
        ? dig_path_segment_impl(p, src_id, dir, len, on_connect, read, always_true {})
        // have to check bounds
        : dig_path_segment_impl(p, src_id, dir, len, on_connect, read, make_bounds_checker_());
}

std::pair<point2i32, bool>
level_impl::find_path_end_point(
    random_state& rng
  , recti32 const region_bounds
) const noexcept {
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
    auto const region_count = regions_.size();

    using vertex_t     = int16_t;
    using graph_data_t = int8_t;

    auto graph      = adjacency_matrix<vertex_t> {static_cast<int>(region_count)};
    auto graph_data = vertex_data<graph_data_t>  {static_cast<int>(region_count)};

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
        BK_ASSERT(n > 1 && static_cast<size_t>(n) <= region_count);

        size_t   min_component_i = 0;
        vertex_t min_component_n = 0;

        std::tie(min_component_i, std::ignore, min_component_n, std::ignore) =
            count_components(graph_data, component_sizes);

        BK_ASSERT(min_component_n > 0);

        get_component_indicies(min_component_i, min_component_n);
        BK_ASSERT(component_indicies.size() > 0);

        for (vertex_t const i : component_indicies) {
           // components are 1-based
           auto const c      = static_cast<vertex_t>(i + 1);
           auto const off    = find_nth_random(graph_data, min_component_n, c);
           auto const index  = static_cast<size_t>(off);
           auto const src_id =
               region_id {static_cast<uint16_t>(region(index).id)};

           dig_random(rng, region(index).bounds
             , [&](region_id const id, point2i32) noexcept {
                   add_connection(src_id, id);
               });
        }

        return true;
    });
}

void level_impl::generate(random_state& rng) {
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
      , tile_flags {tile_flag::solid}
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

const_sub_region_range<tile_id>
level_impl::update_tile_at(
    random_state&        rng
  , point2i32 const      p
  , tile_data_set const& data
) noexcept {
    auto const r = recti32 {p, sizei32x {1}, sizei32y {1}};
    return update_tile_rect(rng, r, &data);
}

template <typename T>
void level_impl::copy_region(
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

} //namespace boken
