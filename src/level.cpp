#include "level.hpp"
#include "tile.hpp"
#include "bsp_generator.hpp"
#include "random.hpp"
#include "utility.hpp"
#include <vector>

namespace boken {

level::~level() = default;

template <typename T>
axis_aligned_rect<T> random_sub_rect(
    random_state&              rng
  , axis_aligned_rect<T> const r
  , size_type<T>         const min_size
  , size_type<T>         const max_size
  , float                const variance = 6.0f
) {
    //BK_ASSERT(min_size <= max_size);
    //BK_ASSERT(variance > 0.0f);

    auto const w = r.width();
    auto const h = r.height();

    auto const new_size = [&](T const size) {
        auto const min_s = value_cast(min_size);
        if (min_s > size) {
            return size;
        }

        auto const max_s = std::min(size, value_cast(max_size));
        if (max_s - min_s <= 0) {
            return size;
        }

        auto const range  = max_s - min_s;
        auto const median = min_s + range / 2.0f;
        auto const roll   = random_normal(rng, median, range / variance);

        return clamp(round_as<T>(roll), min_s, max_s);
    };

    auto const new_w = new_size(w);
    auto const new_h = new_size(h);

    auto const new_offset = [&](T const size) {
        return (size <= 0)
          ? T {0}
          : static_cast<T>(random_uniform_int(rng, 0, static_cast<int>(size)));
    };

    return {offset_type_x<T> {r.x0 + new_offset(w - new_w)}
          , offset_type_y<T> {r.y0 + new_offset(h - new_h)}
          , size_type_x<T>   {new_w}
          , size_type_y<T>   {new_h}};
}

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

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // level interface
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    item const* find(item_instance_id const id) const noexcept final override {
        return nullptr;
    }

    entity const* find(entity_instance_id const id) const noexcept final override {
        return nullptr;
    }

    int region_count() const noexcept final override {
        return bsp_gen_->size();
    }

    tile_view at(int const x, int const y) const noexcept final override {
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

    template <typename T, typename U>
    void fill_rect(std::vector<T>& v, sizeix const width, axis_aligned_rect<U> const r, T const value) noexcept {
        for (auto y = r.y0; y < r.y1; ++y) {
            std::fill_n(v.data() + (r.x0 + y * value_cast(width)), r.width(), value);
        }
    }

    //template <typename T>
    //void fill_line(std::vector<T>& v, sizeix const width, point2i const p, sizeix const w, T const value) noexcept {
    //    size_t const offset = value_cast<size_t>(p.x)
    //                        + value_cast<size_t>(p.y) * value_cast(width));
    //    std::fill_n(v.data() + offset, value_cast(w), value);
    //}

    //template <typename T>
    //void fill_line(std::vector<T>& v, sizeix const width, point2i const p, sizeiy const h, T const value) noexcept {
    //    for (auto y = value_cast<size_t>(p.y); y < value_cast(h); ++y) {
    //        size_t const offset = value_cast<size_t>(p.x) + y * value_cast(width));
    //        v[offset] = value;
    //    }
    //}

    void generate_floor_at(int const x, int const y) noexcept {
        data_at_(data_.types, x, y, width_) = tile_type::floor;
        data_at_(data_.flags, x, y, width_) = tile_flags {0};
    }

    //void generate_floor_at(recti const r) noexcept {
    //    fill_line(data_.types, width_, point2i {r.x0, r.y0},     sizeix {r.width()},      tile_type::floor);
    //    fill_line(data_.types, width_, point2i {r.x0, r.y0 + 1}, sizeix {r.height() - 2}, tile_type::floor);
    //    fill_line(data_.types, width_, point2i {r.x1, r.y0 + 1}, sizeix {r.height() - 2}, tile_type::floor);
    //    fill_line(data_.types, width_, point2i {r.x0, r.y1},     sizeix {r.width()},      tile_type::floor);

    //    fill_line(data_.flags, width_, point2i {r.x0, r.y0},     sizeix {r.width()},      tile_flags {0});
    //    fill_line(data_.flags, width_, point2i {r.x0, r.y0 + 1}, sizeix {r.height() - 2}, tile_flags {0});
    //    fill_line(data_.flags, width_, point2i {r.x1, r.y0 + 1}, sizeix {r.height() - 2}, tile_flags {0});
    //    fill_line(data_.flags, width_, point2i {r.x0, r.y1},     sizeix {r.width()},      tile_flags {0});
    //}

    //void generate_wall_at(recti const r) noexcept {
    //    fill_rect(data_.types, width_, r, tile_type::wall);
    //    fill_rect(data_.flags, width_, r, tile_flags {tile_flags::f_solid});
    //}

    template <typename T>
    void generate_room_data(std::vector<T>& v, recti const r, T const& floor, T const& wall) noexcept {
        auto const at = [&](auto const x, auto const y) noexcept {
            return v.data() + x + y * value_cast<size_t>(width_);
        };

        auto const w = static_cast<size_t>(r.width());

        std::fill_n(at(r.x0, r.y0), w, wall);
        for (auto y = r.y0 + 1; y < r.y1 - 1; ++y) {
            *at(r.x0, y) = wall;
            std::fill_n(at(r.x0 + 1, y), w - 2, floor);
            *at(r.x1 - 1, y) = wall;
        }
        std::fill_n(at(r.x0, r.y1 - 1), w, wall);
    }

    void generate_rooms(random_state& rng) {
    }

    void generate_merge_walls(random_state& rng) {
        using namespace container_algorithms;
        sort(room_rects_, rect_by_min_dimension<int>);

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

        for (auto r : room_rects_) {
            for_each_xy(r, [&](int const x, int const y, bool const on_edge) noexcept {
                if (on_edge && can_remove_shared_wall(r, x, y)) {
                    generate_floor_at(x, y);
                }
            });
        }
    }

    void generate_connections(random_state& rng) {
    }

    void generate_select_ids(random_state& rng) {
    }

    void generate(random_state& rng) {
        {
            recti const map_rect {offix {0}, offiy {0}, width_, height_};
            fill_rect(data_.types, width_, map_rect, tile_type::empty);
            fill_rect(data_.flags, width_, map_rect, tile_flags {tile_flags::f_solid});
        }

        auto& bsp = *bsp_gen_;

        bsp.clear();
        bsp.generate(rng);

        auto const& p = bsp.params();

        auto const room_min_size       = p.min_room_size;
        auto const room_max_size       = p.max_room_size;
        auto const room_gen_chance_num = p.room_chance_num;
        auto const room_gen_chance_den = p.room_chance_den;

        room_rects_.reserve(
            (bsp.size() * value_cast(room_gen_chance_num))
                        / value_cast(room_gen_chance_den));

        uint16_t region_id = 0;
        for (auto const& region : bsp) {
            fill_rect(data_.region_ids, width_, region.rect, region_id++);

            if (!random_chance_in_x(rng, value_cast(room_gen_chance_num)
                                       , value_cast(room_gen_chance_den))
            ) {
                continue;
            }

            auto const room_rect = random_sub_rect(
                rng, region.rect, room_min_size, room_max_size);

            room_rects_.push_back(room_rect);

            generate_room_data(data_.types, room_rect
              , tile_type::floor
              , tile_type::wall);

            generate_room_data(data_.flags, room_rect
              , tile_flags {0}
              , tile_flags {tile_flags::f_solid});
        }

        generate_merge_walls(rng);
    }
private:
    template <typename Vector>
    static auto data_at_(
        Vector&& v, int const x, int const y, sizeix const w
    ) noexcept -> decltype(v[0]) {
        auto const i = static_cast<size_t>(x)
                     + static_cast<size_t>(y) * value_cast<size_t>(w);

        return v[i];
    }

    bool check_bounds_(int const x, int const y) const noexcept {
       return (x >= 0)                  && (y >= 0)
           && (x <  value_cast(width_)) && (y <  value_cast(height_));
    }

    sizeix width_;
    sizeiy height_;

    std::unique_ptr<bsp_generator> bsp_gen_;
    std::vector<recti> room_rects_;

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
