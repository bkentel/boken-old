#include "level.hpp"

#include "bsp_generator.hpp"    // for bsp_generator, etc
#include "entity.hpp"           // for entity
#include "random.hpp"           // for random_state (ptr only), etc
#include "tile.hpp"             // for tile_data_set, tile_type, tile_flags, etc
#include "utility.hpp"          // for find_if

#include <bkassert/assert.hpp>  // for BK_ASSERT

#include <algorithm>            // for max, find_if, fill, max_element, min, etc
#include <functional>           // for reference_wrapper, ref
#include <iterator>             // for begin, end, back_insert_iterator, etc
#include <numeric>
#include <vector>               // for vector

#include <cstdint>              // for uint16_t, int32_t

namespace boken {

level::~level() = default;

struct always_true {
    template <typename... Args>
    inline constexpr bool operator()(Args...) const noexcept {
        return true;
    }
};

template <size_t N, typename Check, typename Predicate>
unsigned fold_neighbors_impl(
    std::array<int, N> const& xi
  , std::array<int, N> const& yi
  , int const x
  , int const y
  , Check check
  , Predicate pred
) {
    unsigned result {};

    for (size_t i = 0; i < N; ++i) {
        auto const x0 = x + xi[i];
        auto const y0 = y + yi[i];
        auto const bit = check(x0, y0) && pred(x0, y0) ? 1u : 0u;
        result |= bit << (N - 1 - i);
    }

    return result;
}

//     N[3]
// W[2]    E[1]
//     S[0]
template <typename Check, typename Predicate>
unsigned fold_neighbors4(int const x, int const y, Check check, Predicate pred) {
    constexpr std::array<int, 4> yi {-1,  0, 0, 1};
    constexpr std::array<int, 4> xi { 0, -1, 1, 0};
    return fold_neighbors_impl(xi, yi, x, y, check, pred);
}

// NW[7] N[6] NE[5]
//  W[4]       E[3]
// SW[2] S[1] SE[0]
template <typename Check, typename Predicate>
unsigned fold_neighbors9(int const x, int const y, Check check, Predicate pred) {
    constexpr std::array<int, 8> yi {-1, -1, -1,  0, 0,  1, 1, 1};
    constexpr std::array<int, 8> xi {-1,  0,  1, -1, 1, -1, 0, 1};
    return fold_neighbors_impl(xi, yi, x, y, check, pred);
}

template <typename T>
constexpr axis_aligned_rect<T> shrink_rect(axis_aligned_rect<T> const r) noexcept {
    return {offset_type_x<T> {r.x0 + 1}, offset_type_y<T> {r.y0 + 1}
          , offset_type_x<T> {r.x1 - 1}, offset_type_y<T> {r.y1 - 1}};
}

template <typename T>
constexpr axis_aligned_rect<T> grow_rect(axis_aligned_rect<T> const r) noexcept {
    return {offset_type_x<T> {r.x0 - 1}, offset_type_y<T> {r.y0 - 1}
          , offset_type_x<T> {r.x1 + 1}, offset_type_y<T> {r.y1 + 1}};
}

//
// 1111111111
// 2000000002
// 2000000002
// 2000000002
// 3333333333
//
template <typename T, typename CenterF, typename EdgeF>
void for_each_xy_center_first(axis_aligned_rect<T> const r, CenterF center, EdgeF edge) {
    // inner tiles
    for_each_xy(shrink_rect(r), center);

    //top edge
    for (auto x = r.x0; x < r.x1; ++x) {
        edge(x, r.y0);
    }

    //left and right edge
    for (auto y = r.y0 + 1; y < r.y1 - 1; ++y) {
        edge(r.x0,     y);
        edge(r.x1 - 1, y);
    }

    // bottom edge
    for (auto x = r.x0; x < r.x1; ++x) {
        edge(x, r.y1 - 1);
    }
}

template <typename Vector>
auto& data_at(Vector&& v, int const x, int const y, sizeix const w) noexcept {
    auto const i = static_cast<size_t>(x)
                 + static_cast<size_t>(y) * value_cast<size_t>(w);

    return v[i];
}

template <typename Vector>
auto& data_at(Vector&& v, point2i const p, sizeix const w) noexcept {
    return data_at(std::forward<Vector>(v), value_cast(p.x), value_cast(p.y), w);
}

template <typename T, typename Predicate>
std::pair<point2<T>, bool> find_random_nearest(
    random_state&         rng
  , point2<T> const       origin
  , T const               max_distance
  , std::vector<point2i>& points
  , Predicate             pred
) {
    BK_ASSERT(max_distance >= 0);

    points.clear();
    points.reserve(static_cast<size_t>(max_distance * 8));

    auto const ox = value_cast(origin.x);
    auto const oy = value_cast(origin.y);

    for (T d = 0; d <= max_distance; ++d) {
        for (T i = 0; i < d * 2; ++i) {
            points.push_back({ox - d, 0 - d + i});
            points.push_back({ox + d, 0 - d + i});
        }

        for (T i = 1; i < d * 2 - 1; ++i) {
            points.push_back({0 - d + i, oy - d});
            points.push_back({0 - d + i, oy + d});
        }

        std::shuffle(begin(points), end(points), rng);

        auto const it = std::find_if(begin(points), end(points), pred);
        if (it != std::end(points)) {
            return {*it, true};
        }
    }

    return {origin, false};
}

template <typename T, typename Predicate>
auto find_random_nearest(random_state& rng, point2<T> const origin, T const max_distance, Predicate pred) {
    std::vector<point2i> points;
    return find_random_nearest(rng, origin, max_distance, points, pred);
}

template <typename T>
constexpr axis_aligned_rect<T> move_to_origin(axis_aligned_rect<T> const r) noexcept {
    return axis_aligned_rect<T> {
        offset_type_x<T> {0}
      , offset_type_y<T> {0}
      , size_type_x<T>   {r.width()}
      , size_type_y<T>   {r.height()}
    };
}

struct generate_rect_room {
    template <typename T>
    static axis_aligned_rect<T> random_sub_rect(
        random_state&              rng
      , axis_aligned_rect<T> const r
      , size_type<T>         const min_size
      , size_type<T>         const max_size
      , double               const inverse_variance = 6.0
    ) noexcept {
        BK_ASSERT(min_size <= max_size);
        BK_ASSERT(inverse_variance > 0.0f);

        auto const w = r.width();
        auto const h = r.height();

        auto const new_size = [&](T const size) noexcept {
            auto const min_s = value_cast(min_size);
            if (min_s > size) {
                return size;
            }

            auto const max_s = std::min(size, value_cast(max_size));
            if (max_s - min_s <= 0) {
                return size;
            }

            auto const range    = max_s - min_s;
            auto const median   = min_s + range / 2.0;
            auto const variance = range / inverse_variance;
            auto const roll     = random_normal(rng, median, variance);

            return clamp(round_as<T>(roll), min_s, max_s);
        };

        auto const new_w = new_size(w);
        auto const new_h = new_size(h);

        auto const new_offset = [&](T const size) noexcept {
            return static_cast<T>((size <= 0)
              ? 0 : random_uniform_int(rng, 0, size));
        };

        return {offset_type_x<T> {r.x0 + new_offset(w - new_w)}
              , offset_type_y<T> {r.y0 + new_offset(h - new_h)}
              , size_type_x<T>   {new_w}
              , size_type_y<T>   {new_h}};
    }

    generate_rect_room(sizei const room_min_size, sizei const room_max_size) noexcept
      : room_min_size_ {room_min_size}
      , room_max_size_ {room_max_size}
    {
    }

    int32_t operator()(random_state& rng, recti const area, std::vector<tile_data_set>& out) {
        int32_t count = 0;

        auto const r = random_sub_rect(rng, move_to_origin(area), room_min_size_, room_max_size_);
        auto const area_w = area.width();
        auto const room_w = r.width();
        auto const step   = area_w - room_w;

        auto it = begin(out) + (r.x0) + (r.y0 * area_w);
        for (auto y = r.y0; y < r.y1; ++y) {
            bool const on_edge_y = (y == r.y0) || (it += step, false) || (y == r.y1 - 1);
            for (auto x = r.x0; x < r.x1; ++x, ++it) {
                bool const on_edge = on_edge_y || (x == r.x0) || (x == r.x1 - 1);
                if (on_edge) {
                    it->type  = tile_type::wall;
                    it->flags = tile_flags {tile_flags::f_solid};
                } else {
                    it->type  = tile_type::floor;
                    it->flags = tile_flags {};
                }
                ++count;
            }
        }

        return count;
    }

    sizei room_min_size_;
    sizei room_max_size_;
};

class level_impl : public level {
public:
    level_impl(random_state& rng, sizeix const width, sizeiy const height)
      : bounds_  {offix {0}, offiy {0}, width, height}
      , data_    {width, height}
    {
        bsp_generator::param_t p;
        p.width  = sizeix {width};
        p.height = sizeiy {height};
        p.min_room_size = sizei {3};
        p.room_chance_num = sizei {80};

        bsp_gen_ = make_bsp_generator(p);
        generate(rng);
    }

    auto entity_at_(point2i const p) const noexcept {
        return std::find_if(
            begin(entities_.positions)
          , end(entities_.positions)
          , [p](auto const& q) {
                return q == p;
            });
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // level interface
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    sizeix width() const noexcept final override {
        return sizeix {bounds_.width()};
    }

    sizeiy height() const noexcept final override {
        return sizeiy {bounds_.height()};
    }

    recti bounds() const noexcept final override {
        return bounds_;
    }

    item const* find(item_instance_id const id) const noexcept final override {
        return nullptr;
    }

    entity const* find(entity_instance_id const id) const noexcept final override {
        return nullptr;
    }

    entity const* entity_at(point2i const p) const noexcept final override {
        auto const it = entity_at_(p);
        if (it == end(entities_.positions)) {
            return nullptr;
        }

        auto const i = std::distance(begin(entities_.positions), it);
        return entities_.intances.data() + i;
    }

    item const* item_at(point2i const p) const noexcept final override {
        return nullptr;
    }

    placement_result can_place_entity_at(point2i const p) const noexcept final override {
        return !check_bounds_(p)
                 ? placement_result::failed_bounds
             : data_at_(data_.flags, p).test(tile_flags::f_solid)
                 ? placement_result::failed_obstacle
             : is_entity_at(p)
                 ? placement_result::failed_entity
                 : placement_result::ok;
    }

    placement_result can_place_item_at(point2i p) const noexcept final override {
        return placement_result::failed_obstacle;
    }

    placement_result move_by(item_instance_id const id, vec2i const v) noexcept final override {
        return placement_result::ok;
    }

    placement_result move_entity_by_(size_t const i, vec2i const v) noexcept {
        auto const p = entities_.positions[i].cast_to<int>() + v;
        auto const result = can_place_entity_at(p);
        if (result == placement_result::ok) {
            entities_.positions[i] = p.cast_to<uint16_t>();
        }

        return result;
    }

    placement_result move_by(entity_instance_id const id, vec2i const v) noexcept final override {
        using namespace container_algorithms;
        auto const it = find_if(entities_.intances
          , [id](entity const& e) noexcept { return id == e.instance(); });

        if (it == end(entities_.intances)) {
            return placement_result::failed_bad_id;
        }

        return move_entity_by_(std::distance(begin(entities_.intances), it), v);
    }

    void transform_entities(std::function<point2i (entity&, point2i)>&& f) final override {
        size_t i = 0;
        for (auto& e : entities_.intances) {
            auto const p = entities_.positions[i].cast_to<int>();
            auto const q = f(e, p);

            if (p != q) {
                move_entity_by_(i, q - p);
            }

            ++i;
        }
    }

    placement_result add_item_at(item&& i, point2i p) final override {
        return placement_result::failed_obstacle;
    }

    placement_result add_entity_at(entity&& e, point2i const p) final override {
        auto const result = can_place_entity_at(p);
        if (result != placement_result::ok) {
            return result;
        }

        entities_.ids.push_back(e.definition());
        entities_.positions.push_back(p.cast_to<uint16_t>());
        entities_.intances.push_back(std::move(e));

        return placement_result::ok;
    }

    std::pair<point2i, placement_result>
    add_item_nearest_random(random_state& rng, item&& i, point2i p, int max_distance) final override {
        return {p, placement_result::failed_obstacle};
    }

    std::pair<point2i, placement_result>
    add_entity_nearest_random(random_state& rng, entity&& e, point2i const p, int const max_distance) final override {
        auto const where = find_random_nearest(rng, p, max_distance
          , [&](point2i const q) {
                return add_entity_at(std::move(e), q) == placement_result::ok;
            });

        if (!where.second) {
            return {p, placement_result::failed_obstacle};
        }

        return {where.first, placement_result::ok};
    }

    size_t region_count() const noexcept final override {
        return bsp_gen_->size();
    }

    region_info region(size_t const i) const noexcept final override {
        BK_ASSERT(i < regions_.size());
        return regions_[i];
    }

    tile_view at(point2i const p) const noexcept final override {
        auto const x = value_cast(p.x);
        auto const y = value_cast(p.y);

        if (!check_bounds_(x, y)) {
            static tile_id    const dummy_id         {};
            static tile_type  const dummy_type       {tile_type::empty};
            static tile_flags const dummy_flags      {};
            static uint16_t   const dummy_region_id  {};
            static tile_data* const dummy_data       {};

            return {
                dummy_id
              , dummy_type
              , dummy_flags
              , dummy_region_id
              , dummy_data
            };
        }

        return {
            data_at_(data_.ids,        x, y)
          , data_at_(data_.types,      x, y)
          , data_at_(data_.flags,      x, y)
          , data_at_(data_.region_ids, x, y)
          , nullptr
        };
    }

    std::vector<point2<uint16_t>> const& entity_positions() const noexcept final override {
        return entities_.positions;
    }

    std::vector<entity_id> const& entity_ids() const noexcept final override {
        return entities_.ids;
    }

    const_sub_region_range<tile_id>
    tile_ids(recti const area) const noexcept final override {
        auto const b = bounds();
        auto const r = clamp(area, b);

        return make_sub_region_range(as_const(data_.ids.data())
          , r.x0,      r.y0
          , b.width(), b.height()
          , r.width(), r.height());
    }

    const_sub_region_range<uint16_t>
    region_ids(recti const area) const noexcept final override {
        auto const b = bounds();
        auto const r = clamp(area, b);

        return make_sub_region_range(as_const(data_.region_ids.data())
          , r.x0,      r.y0
          , b.width(), b.height()
          , r.width(), r.height());
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // implementation
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    template <typename T, typename U>
    void fill_rect(
        std::vector<T>& v
      , sizeix const width
      , axis_aligned_rect<U> const r
      , T const value
    ) noexcept {
        auto p = v.data() + r.x0 + r.y0 * value_cast(width);
        auto const step = value_cast(width) - r.width();
        for (auto y = r.y0; y < r.y1; ++y) {
            p = std::fill_n(p, r.width(), value) + step;
        }
    }

    template <typename T>
    void copy_region(
        tile_data_set const*     const src
      , T const tile_data_set::* const src_field
      , recti const                    src_rect
      , std::vector<T>&                dst
    ) noexcept {
        auto const src_w = src_rect.width();
        auto const dst_w = value_cast(width());
        auto const step = static_cast<size_t>(dst_w - src_w);

        BK_ASSERT(src_w <= dst_w);

        auto src_off = size_t {0};
        auto dst_off = static_cast<size_t>(src_rect.x0 + src_rect.y0 * dst_w);

        for (auto y = src_rect.y0; y < src_rect.y1; ++y, dst_off += step) {
            for (auto x = src_rect.x0; x < src_rect.x1; ++x, ++src_off, ++dst_off) {
                dst[dst_off] = src[src_off].*src_field;
            }
        }
    }

    void place_doors(random_state& rng, recti area);

    void merge_walls_at(random_state& rng, recti area);

    void update_tile_ids(random_state& rng, recti area);

    void generate_make_connections(random_state& rng) {
        constexpr std::array<int, 4> dir_x {-1,  0, 0, 1};
        constexpr std::array<int, 4> dir_y { 0, -1, 1, 0};

        for (auto const& region : regions_) {
            if (region.tile_count <= 0) {
                continue;
            }

            auto const& bounds = region.bounds;
            auto p = point2i {bounds.x0 + bounds.width()  / 2
                            , bounds.y0 + bounds.height() / 2};

            auto const segments = random_uniform_int(rng, 0, 10);
            for (int i = 0; i < segments; ++i) {
                auto const dir = random_uniform_int(rng, 0, 3);
                auto const d   = vec2i {dir_x[dir], dir_y[dir]};

                for (auto len = random_uniform_int(rng, 3, 10); len > 0; --len) {
                    auto const p0 = p + d;
                    if (!check_bounds_(p0)) {
                        break;
                    }

                    auto& type = data_at_(data_.types, p0);
                    switch (type) {
                    case tile_type::empty:
                        type = tile_type::tunnel;
                        data_at_(data_.flags, p0) = tile_flags {0};
                        break;
                    case tile_type::wall:
                        type = tile_type::floor;
                        data_at_(data_.flags, p0) = tile_flags {0};
                        break;
                    case tile_type::floor  : break;
                    case tile_type::tunnel : break;
                    case tile_type::door   : break;
                    case tile_type::stair  : break;
                    default                : break;
                    }

                    p = p0;
                }
            }
        }
    }

    void generate(random_state& rng) {
        std::fill(begin(data_.types), end(data_.types), tile_type::empty);
        std::fill(begin(data_.flags), end(data_.flags), tile_flags {tile_flags::f_solid});

        auto&       bsp = *bsp_gen_;
        auto const& p   = bsp.params();

        bsp.clear();
        bsp.generate(rng);

        regions_.clear();
        regions_.reserve(bsp.size());
        std::transform(std::begin(bsp), std::end(bsp), back_inserter(regions_)
          , [](auto const& node) noexcept {
                return region_info {node.rect, 0, 0, 0};
            });

        std::vector<tile_data_set> buffer;
        {
            auto const size = std::max_element(std::begin(bsp), std::end(bsp)
              , [](auto const& node_a, auto const& node_b) noexcept {
                    return node_a.rect.area() < node_b.rect.area();
                })->rect.area();

            buffer.reserve(static_cast<size_t>(std::max(0, size)));
        }

        tile_data_set default_tile {
            tile_data  {}
          , tile_flags {tile_flags::f_solid}
          , tile_id    {}
          , tile_type  {tile_type::empty}
          , uint16_t   {}
        };

        auto gen_rect = generate_rect_room {p.min_room_size, p.max_room_size};

        auto const roll_room_chance = [&]() noexcept {
            return random_chance_in_x(rng, value_cast(p.room_chance_num)
                                         , value_cast(p.room_chance_den));
        };

        for (auto& region : regions_) {
            auto const rect = region.bounds;
            fill_rect(data_.region_ids, width(), rect, default_tile.region_id++);

            if (!roll_room_chance()) {
                continue;
            }

            buffer.resize(static_cast<size_t>(std::max(0, rect.area())), default_tile);
            region.tile_count = gen_rect(rng, rect, buffer);

            copy_region(buffer.data(), &tile_data_set::id,    rect, data_.ids);
            copy_region(buffer.data(), &tile_data_set::type,  rect, data_.types);
            copy_region(buffer.data(), &tile_data_set::flags, rect, data_.flags);

            buffer.resize(0);
        }

        generate_make_connections(rng);
        merge_walls_at(rng, bounds_);
        place_doors(rng, bounds_);
        update_tile_ids(rng, bounds_);
    }

    const_sub_region_range<tile_id> update_tile_rect(random_state& rng, recti const area, tile_data_set const* const data);

    const_sub_region_range<tile_id> update_tile_at(random_state& rng, point2i const p, tile_data_set const& data) noexcept final override {
        auto const r = recti {p.x, p.y, sizeix {1}, sizeiy{1}};
        return update_tile_rect(rng, r, &data);
    }

    const_sub_region_range<tile_id> update_tile_rect(random_state& rng, recti const area, std::vector<tile_data_set> const& data) {
        return update_tile_rect(rng, area, data.data());
    }
private:
    template <typename Vector>
    auto make_data_reader_(Vector&& v) const noexcept {
        return [&](int const x, int const y) noexcept {
            return data_at_(std::forward<Vector>(v), x, y);
        };
    }

    auto make_bounds_checker_() const noexcept {
        return [&](int const x, int const y) noexcept {
            return check_bounds_(x, y);
        };
    }

    template <typename Vector>
    auto data_at_(Vector&& v, int const x, int const y) const noexcept -> decltype(v[0]) {
        return data_at(std::forward<Vector>(v), x, y, width());
    }

    template <typename Vector>
    auto data_at_(Vector&& v, point2i const p) const noexcept -> decltype(v[0]) {
        return data_at(std::forward<Vector>(v), p, width());
    }

    template <typename Vector>
    auto data_at_(Vector&& v, int const x, int const y) noexcept -> decltype(v[0]) {
        return data_at(std::forward<Vector>(v), x, y, width());
    }

    template <typename Vector>
    auto data_at_(Vector&& v, point2i const p) noexcept -> decltype(v[0]) {
        return data_at(std::forward<Vector>(v), p, width());
    }


    bool check_bounds_(int const x, int const y) const noexcept {
       return (x >= 0)                   && (y >= 0)
           && (x <  value_cast(width())) && (y <  value_cast(height()));
    }

    bool check_bounds_(point2i const p) const noexcept {
        return check_bounds_(value_cast(p.x), value_cast(p.y));
    }

    recti bounds_;

    struct entities_t {
        std::vector<point2<uint16_t>> positions;
        std::vector<entity_id>        ids;
        std::vector<entity>           intances;
    } entities_;

    std::unique_ptr<bsp_generator> bsp_gen_;
    std::vector<region_info> regions_;

    struct data_t {
        explicit data_t(size_t const size)
          : ids(size)
          , types(size)
          , flags(size)
          , region_ids(size)
        {
        }

        data_t(sizeix const width, sizeiy const height)
          : data_t {value_cast<size_t>(width) * value_cast<size_t>(height)}
        {
        }

        std::vector<tile_id>    ids;
        std::vector<tile_type>  types;
        std::vector<tile_flags> flags;
        std::vector<uint16_t>   region_ids;
    } data_;
};

std::unique_ptr<level> make_level(random_state& rng, sizeix const width, sizeiy const height) {
    return std::make_unique<level_impl>(rng, width, height);
}

bool can_remove_wall_code(
    unsigned const wall_type
  , unsigned const other_type
) noexcept {
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

template <typename Read, typename Check>
bool can_remove_wall_at(int const x, int const y, Read read, Check check) noexcept {
    static_assert(noexcept(read(x, y)), "");
    static_assert(noexcept(check(x, y)), "");

    auto const wall_type =
        fold_neighbors9(x, y, check, [&](int const x0, int const y0) noexcept {
            return read(x0, y0) == tile_type::wall;
        });

    auto const other_type =
        fold_neighbors9(x, y, check, [&](int const x0, int const y0) noexcept {
            return read(x0, y0) == tile_type::floor;
        });

    return can_remove_wall_code(wall_type, other_type);
}

tile_id wall_code_to_id(unsigned const code) noexcept {
    using ti = tile_id;
    switch (code) {
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

template <typename Read, typename Check>
tile_id tile_type_to_id_at(int const x, int const y, Read read, Check check) noexcept {
    static_assert(noexcept(read(x, y)), "");
    static_assert(noexcept(check(x, y)), "");

    using ti = tile_id;
    using tt = tile_type;

    switch (read(x, y)) {
    case tt::empty  : return ti::empty;
    case tt::floor  : return ti::floor;
    case tt::tunnel : return ti::tunnel;
    case tt::door   : break;
    case tt::stair  : break;
    case tt::wall   : return wall_code_to_id(
        fold_neighbors4(x, y, check, [&](int const x0, int const y0) noexcept {
            auto const type = read(x0, y0);
            return type == tt::wall || type == tt::door;
        }));
    }

    return ti::invalid;
}

template <typename Read, typename Check>
tile_id can_place_door_at(int const x, int const y, Read read, Check check) noexcept {
    static_assert(noexcept(read(x, y)), "");
    static_assert(noexcept(check(x, y)), "");

    auto const wall_type =
        fold_neighbors4(x, y, check, [&](int const x0, int const y0) noexcept {
            return read(x0, y0) == tile_type::wall;
        });

    auto const other_type =
        fold_neighbors4(x, y, check, [&](int const x0, int const y0) noexcept {
            auto const type = read(x0, y0);
            return type == tile_type::floor || type == tile_type::tunnel;
        });

    return (wall_type == 0b1001 && other_type == 0b0110) ? tile_id::door_ns_closed
         : (wall_type == 0b0110 && other_type == 0b1001) ? tile_id::door_ew_closed
         : tile_id::invalid;
}

template <typename Check, typename Transform>
void transform_area(recti const area, recti const bounds, Check check, Transform transform) {
    auto const transform_checked = [&](int const x, int const y) noexcept {
        transform(x, y, check);
    };

    auto const transform_unchecked = [&](int const x, int const y) noexcept {
        transform(x, y, always_true {});
    };

    bool const must_check = area.x0 == bounds.x0
                         || area.y0 == bounds.y0
                         || area.x1 == bounds.x1 - 1
                         || area.y1 == bounds.y1 - 1;

    if (must_check) {
        for_each_xy_center_first(area, transform_unchecked, transform_checked);
    } else {
        for_each_xy_center_first(area, transform_unchecked, transform_unchecked);
    }
}

void level_impl::merge_walls_at(random_state& rng, recti const area) {
    auto const read = make_data_reader_(data_.types);
    transform_area(area, bounds_, make_bounds_checker_()
      , [&](int const x, int const y, auto check) noexcept {
            // TODO: explicit 'this' due to a GCC bug (5.2.1)
            auto& type = this->data_at_(data_.types, x, y);
            if (type != tile_type::wall || !can_remove_wall_at(x, y, read, check)) {
                return;
            }

            type = tile_type::floor;
            this->data_at_(data_.flags, x, y) = tile_flags {0};
        }
    );
}

void level_impl::update_tile_ids(random_state& rng, recti const area) {
    auto const read = make_data_reader_(data_.types);
    transform_area(area, bounds_, make_bounds_checker_()
      , [&](int const x, int const y, auto check) noexcept {
            // TODO: explicit 'this' due to a GCC bug (5.2.1)
            auto const id = tile_type_to_id_at(x, y, read, check);
            if (id != tile_id::invalid) {
                this->data_at_(data_.ids, x, y) = id;
            }
        }
    );
}

void level_impl::place_doors(random_state& rng, recti const area) {
    auto const read = make_data_reader_(data_.types);
    transform_area(area, bounds_, make_bounds_checker_()
      , [&](int const x, int const y, auto check) noexcept {
            auto const id = can_place_door_at(x, y, read, check);
            if (id == tile_id::invalid || random_coin_flip(rng)) {
                return;
            }

            // TODO: explicit 'this' due to a GCC bug (5.2.1)
            this->data_at_(data_.types, x, y) = tile_type::door;
            this->data_at_(data_.ids, x, y) = id;
            this->data_at_(data_.flags, x, y) = tile_flags {tile_flags::f_solid};
        }
    );
}

const_sub_region_range<tile_id>
level_impl::update_tile_rect(
    random_state&              rng
  , recti                const area
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
      , update_area.x0,      update_area.y0
      , bounds_.width(),     bounds_.height()
      , update_area.width(), update_area.height()
    );
}

} //namespace boken
