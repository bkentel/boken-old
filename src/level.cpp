#include "level.hpp"
#include "tile.hpp"
#include "bsp_generator.hpp"
#include "random.hpp"
#include "utility.hpp"
#include "entity.hpp"
#include <bkassert/assert.hpp>
#include <vector>

namespace boken {

level::~level() = default;

template <typename T, typename Predicate>
std::pair<point2<T>, bool> find_random_nearest(
    random_state&         rng
  , point2<T> const       origin
  , T const               max_distance
  , std::vector<point2i>& points
  , Predicate             pred
) {
    points.clear();
    points.reserve(max_distance * 8);

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

template <typename T, typename F>
void for_each_xy(axis_aligned_rect<T> const r, F f) {
    for (auto y = r.y0; y < r.y1; ++y) {
        bool const on_edge_y = (y == r.y0) || (y == r.y1 - 1);
        for (auto x = r.x0; x < r.x1; ++x) {
            bool const on_edge = on_edge_y || (x == r.x0) || (x == r.x1 - 1);
            f(x, y, on_edge);
        }
    }
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
      , float                const inverse_variance = 6.0f
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

            auto const range  = max_s - min_s;
            auto const median = min_s + range / 2.0;
            auto const roll   = random_normal(rng, median, range / inverse_variance);

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
      : width_  {width}
      , height_ {height}
      , data_   {width, height}
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
        return width_;
    }

    sizeiy height() const noexcept final override {
        return height_;
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
             : data_at_(data_.flags, p, width_).test(tile_flags::f_solid)
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

    placement_result move_by(entity_instance_id const id, vec2i const v) noexcept final override {
        using namespace container_algorithms;
        auto const it = find_if(entities_.intances
          , [id](entity const& e) noexcept { return id == e.instance_id; });

        if (it == end(entities_.intances)) {
            return placement_result::failed_bad_id;
        }

        auto const i = std::distance(begin(entities_.intances), it);
        auto const p = entities_.positions[i].cast_to<int>() + v;

        auto const result = can_place_entity_at(p);
        if (result == placement_result::ok) {
            entities_.positions[i] = p.cast_to<uint16_t>();
        }

        return result;
    }

    placement_result add_item_at(item&& i, point2i p) final override {
        return placement_result::failed_obstacle;
    }

    placement_result add_entity_at(entity&& e, point2i const p) final override {
        auto const result = can_place_entity_at(p);
        if (result != placement_result::ok) {
            return result;
        }

        entities_.ids.push_back(e.id);
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
          , [&](auto const q) {
                return add_entity_at(std::move(e), q) == placement_result::ok;
            });

        if (!where.second) {
            return {p, placement_result::failed_obstacle};
        }

        return {where.first, placement_result::ok};
    }

    int region_count() const noexcept final override {
        return bsp_gen_->size();
    }

    region_info region(int const i) const noexcept final override {
        BK_ASSERT(i >= 0 && static_cast<size_t>(i) < regions_.size());
        return regions_[static_cast<size_t>(i)];
    }

    tile_view at(point2i const p) const noexcept final override {
        auto const x = value_cast(p.x);
        auto const y = value_cast(p.y);

        if (!check_bounds_(x, y)) {
            static tile_id    const dummy_id         {};
            static tile_type  const dummy_type       {tile_type::empty};
            static tile_flags const dummy_flags      {};
            static uint16_t   const dummy_tile_index {};
            static uint16_t   const dummy_region_id  {};
            static tile_data* const dummy_data       {};

            return {
                dummy_id
              , dummy_type
              , dummy_flags
              , dummy_tile_index
              , dummy_region_id
              , dummy_data
            };
        }

        return {
            data_at_(data_.ids,           x, y, width_)
          , data_at_(data_.types,         x, y, width_)
          , data_at_(data_.flags,         x, y, width_)
          , data_at_(data_.tile_indicies, x, y, width_)
          , data_at_(data_.region_ids,    x, y, width_)
          , nullptr
        };
    }

    std::vector<point2<uint16_t>> const& entity_positions() const noexcept final override {
        return entities_.positions;
    }

    std::vector<entity_id> const& entity_ids() const noexcept final override {
        return entities_.ids;
    }

    std::pair<std::vector<uint16_t> const&, recti>
    tile_indicies(int const block) const noexcept final override {
        return std::make_pair(std::ref(data_.tile_indicies)
          , recti {offix {0}, offiy {0}, width_, height_});
    }

    std::pair<std::vector<uint16_t> const&, recti>
    region_ids(int const block) const noexcept final override {
        return std::make_pair(std::ref(data_.region_ids)
          , recti {offix {0}, offiy {0}, width_, height_});
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
        std::vector<tile_data_set> const& src
      , T const tile_data_set::* const    src_field
      , recti const                       src_rect
      , std::vector<T>&                   dst
    ) noexcept {
        auto const src_w = src_rect.width();
        auto const dst_w = value_cast(width_);
        auto const step  = dst_w - src_w;

        auto src_off = 0;
        auto dst_off = src_rect.x0 + src_rect.y0 * dst_w;

        for (auto y = src_rect.y0; y < src_rect.y1; ++y, dst_off += step) {
            for (auto x = src_rect.x0; x < src_rect.x1; ++x, ++src_off, ++dst_off) {
                dst[dst_off] = src[src_off].*src_field;
            }
        }
    }

    void generate_merge_walls(random_state& rng) {
        using namespace container_algorithms;
        //sort(room_rects_, rect_by_min_dimension<int>);

        auto const can_remove_shared_wall = [&](recti const r, int const x, int const y) noexcept {
            auto const is_wall = [&](int const xi, int const yi) noexcept {
                return check_bounds_(xi, yi)
                    && data_at_(data_.types, xi, yi, width_) == tile_type::wall;
            };

            auto const type = ((x == r.x0)     ? 0b0001u : 0u)
                            | ((x == r.x1 - 1) ? 0b0010u : 0u)
                            | ((y == r.y0)     ? 0b0100u : 0u)
                            | ((y == r.y1 - 1) ? 0b1000u : 0u);

            switch (type) {
            case 0b0001 : // left
                return is_wall(x - 1, y - 1)
                    && is_wall(x - 1, y    )
                    && is_wall(x - 1, y + 1);
            case 0b0010 : // right
                return is_wall(x + 1, y - 1)
                    && is_wall(x + 1, y    )
                    && is_wall(x + 1, y + 1);
            case 0b0100 : // top
                return is_wall(x - 1, y - 1)
                    && is_wall(x    , y - 1)
                    && is_wall(x + 1, y - 1);
            case 0b1000 : // bottom
                return is_wall(x - 1, y + 1)
                    && is_wall(x    , y + 1)
                    && is_wall(x + 1, y + 1);
            case 0b0101 : // top left
                return is_wall(x - 1, y - 1)
                    && is_wall(x - 1, y    )
                    && is_wall(x - 1, y + 1)
                    && is_wall(x    , y - 1)
                    && is_wall(x + 1, y - 1);
            case 0b0110 : // top right
                return is_wall(x + 1, y - 1)
                    && is_wall(x + 1, y    )
                    && is_wall(x + 1, y + 1)
                    && is_wall(x    , y - 1)
                    && is_wall(x - 1, y - 1);
            case 0b1001 : // bottom left
                return is_wall(x - 1, y - 1)
                    && is_wall(x - 1, y    )
                    && is_wall(x - 1, y + 1)
                    && is_wall(x    , y + 1)
                    && is_wall(x + 1, y + 1);
            case 0b1010 : // bottom right
                return is_wall(x + 1, y - 1)
                    && is_wall(x + 1, y    )
                    && is_wall(x + 1, y + 1)
                    && is_wall(x    , y + 1)
                    && is_wall(x - 1, y + 1);
            }

            return false;
        };

        //for (auto r : room_rects_) {
        //    for_each_xy(r, [&](int const x, int const y, bool const on_edge) noexcept {
        //        if (on_edge && can_remove_shared_wall(r, x, y)) {
        //            data_at_(data_.types, x, y, width_) = tile_type::floor;
        //            data_at_(data_.flags, x, y, width_) = tile_flags {0};
        //        }
        //    });
        //}
    }

    void generate_select_ids(random_state& rng) {
        recti const map_rect {offix {0}, offiy {0}, width_, height_};
        for_each_xy(map_rect, [&](int const x, int const y, bool) noexcept {
            auto const& type  = data_at_(data_.types, x, y, width_);
            auto&       index = data_at_(data_.tile_indicies, x, y, width_);

            switch (type) {
            case tile_type::empty : index = 11 + 13 * 16; break;
            case tile_type::floor : index = 7;            break;
            case tile_type::wall  : index = 11 * 16;      break;
            }
        });
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
        buffer.reserve(std::max_element(std::begin(bsp), std::end(bsp)
          , [](auto const& node_a, auto const& node_b) noexcept {
                return node_a.rect.area() < node_b.rect.area();
            })->rect.area());

        tile_data_set default_tile {
            tile_data  {}
          , tile_flags {tile_flags::f_solid}
          , tile_id    {}
          , tile_type  {tile_type::empty}
          , uint16_t   {}
          , uint16_t   {}
        };

        auto gen_rect = generate_rect_room {p.min_room_size, p.max_room_size};

        auto const roll_room_chance = [&]() noexcept {
            return random_chance_in_x(rng, value_cast(p.room_chance_num)
                                         , value_cast(p.room_chance_den));
        };

        for (auto& region : regions_) {
            auto const rect = region.bounds;
            fill_rect(data_.region_ids, width_, rect, default_tile.region_id++);

            if (!roll_room_chance()) {
                continue;
            }

            buffer.resize(rect.area(), default_tile);
            region.tile_count = gen_rect(rng, rect, buffer);

            copy_region(buffer, &tile_data_set::id,         rect, data_.ids);
            copy_region(buffer, &tile_data_set::type,       rect, data_.types);
            copy_region(buffer, &tile_data_set::flags,      rect, data_.flags);
            copy_region(buffer, &tile_data_set::tile_index, rect, data_.tile_indicies);

            buffer.resize(0);
        }

        generate_merge_walls(rng);
        generate_select_ids(rng);
    }
private:
    template <typename Vector>
    static auto data_at_(Vector&& v, int const x, int const y, sizeix const w)
        noexcept -> decltype(v[0])
    {
        auto const i = static_cast<size_t>(x)
                     + static_cast<size_t>(y) * value_cast<size_t>(w);

        return v[i];
    }

    template <typename Vector>
    static auto data_at_(Vector&& v, point2i const p, sizeix const w) noexcept
        -> decltype(v[0])
    {
        return data_at_(std::forward<Vector>(v), value_cast(p.x), value_cast(p.y), w);
    }

    bool check_bounds_(int const x, int const y) const noexcept {
       return (x >= 0)                  && (y >= 0)
           && (x <  value_cast(width_)) && (y <  value_cast(height_));
    }

    bool check_bounds_(point2i const p) const noexcept {
        return check_bounds_(value_cast(p.x), value_cast(p.y));
    }

    sizeix width_;
    sizeiy height_;

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
          , tile_indicies(size)
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
        std::vector<uint16_t>   tile_indicies;
        std::vector<uint16_t>   region_ids;
    } data_;
};

std::unique_ptr<level> make_level(random_state& rng, sizeix const width, sizeiy const height) {
    return std::make_unique<level_impl>(rng, width, height);
}

} //namespace boken
