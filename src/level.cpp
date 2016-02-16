#include "level.hpp"
#include "bsp_generator.hpp"
#include "random.hpp"
#include "utility.hpp"
#include <vector>

namespace boken {

template <typename T>
axis_aligned_rect<T> random_sub_rect(
    random_state&              rng
  , axis_aligned_rect<T> const r
  , size_type<T>         const min_size
  , size_type<T>         const max_size
  , float                const variance = 2.0f
) {
    //BK_ASSERT(min_size <= max_size);
    //BK_ASSERT(variance > 0.0f);

    auto const w = r.width();
    auto const h = r.height();

    auto const new_size = [&](T const size) {
        if (size <= value_cast(min_size)) {
            return size;
        }

        auto const max_new_size = std::min(size, value_cast(max_size));
        auto const size_range = max_new_size - value_cast(min_size);
        if (size_range <= 0) {
            return size;
        }

        return clamp(
            round_as<T>(random_normal(
                rng, size_range / 2.0, size_range / variance))
          , value_cast(min_size)
          , max_new_size);
    };

    auto const new_w = new_size(w);
    auto const new_h = new_size(h);

    auto const spare_x = w - new_w;
    auto const spare_y = h - new_h;

    auto const new_offset = [&](T const size) {
        return (size <= 0)
          ? T {0}
          : static_cast<T>(random_uniform_int(rng, 0, static_cast<int>(size)));
    };

    return {offset_type_x<T> {r.x0 + new_offset(spare_x)}
          , offset_type_y<T> {r.y0 + new_offset(spare_y)}
          , size_type_x<T>   {new_w}
          , size_type_y<T>   {new_h}};
}

class level_impl : public level {
public:
    level_impl(random_state& rng, sizeix const width, sizeiy const height)
      : width_  {width}
      , height_ {height}
    {
        size_t const n = value_cast(width) * value_cast(height);
        data_.ids.resize(n);
        data_.flags.resize(n);
        data_.tile_indicies.resize(n);
        data_.region_ids.resize(n);

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
            static tile_flags const dummy_flags      {};
            static uint16_t   const dummy_tile_index {};
            static uint16_t   const dummy_region_id  {};
            static tile_data* const dummy_data       {};

            return {
                dummy_id
              , dummy_flags
              , dummy_tile_index
              , dummy_region_id
              , dummy_data
            };
        }

        return {
            data_at_(data_.ids,           x, y, width_)
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

    void generate(random_state& rng) {
        auto& bsp = *bsp_gen_;

        bsp.clear();
        bsp.generate(rng);

        auto const& p = bsp.params();

        auto const room_min_size = p.min_room_size;
        auto const room_max_size = p.max_room_size;

        auto const room_gen_chance_num = p.room_chance_num;
        auto const room_gen_chance_den = p.room_chance_den;

        std::vector<recti> room_rects;
        room_rects.reserve((bsp.size() * value_cast(room_gen_chance_num))
                         / value_cast(room_gen_chance_den));

        auto const floor_t = std::make_tuple(
            tile_id {uint16_t{1}}, tile_flags {0u}, uint16_t {7u});

        auto const wall_t = std::make_tuple(
            tile_id {uint16_t{2}}, tile_flags {1u}, uint16_t {11u * 16u});

        auto const tie_data = [&](int const x, int const y) noexcept {
            return std::tie(
                data_at_(data_.ids,           x, y, width_)
              , data_at_(data_.flags,         x, y, width_)
              , data_at_(data_.tile_indicies, x, y, width_));
        };

        uint16_t region_id = 0;
        for (auto const& region : bsp) {
            fill_rect(data_.ids,           width_, region.rect, tile_id    {uint16_t{0}});
            fill_rect(data_.flags,         width_, region.rect, tile_flags {1u});
            fill_rect(data_.tile_indicies, width_, region.rect, uint16_t   {11u + 13u * 16u});
            fill_rect(data_.region_ids,    width_, region.rect, uint16_t   {region_id++});

            if (random_uniform_int(rng, 0, value_cast(room_gen_chance_den))
             > value_cast(room_gen_chance_num)
            ) {
                continue;
            }

            auto const room_rect = random_sub_rect(
                rng, region.rect, room_min_size, room_max_size);

            room_rects.push_back(room_rect);

            for_each_xy(room_rect, [&](int const x, int const y, bool const on_edge) noexcept {
                tie_data(x, y) = on_edge ? wall_t : floor_t;
            });
        }

        using namespace container_algorithms;
        sort(room_rects, [](recti r0, recti r1) noexcept {
            return std::min(r0.width(), r0.height())
                 < std::min(r1.width(), r1.height());
        });

        auto const can_remove_shared_wall = [&](recti const r, int const x, int const y) noexcept {
            auto const is_wall = [&](int const xi, int const yi) noexcept {
                return check_bounds_(xi, yi)
                    && data_at_(data_.ids, xi, yi, width_) == tile_id {uint16_t {2}};
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

        for (auto r : room_rects) {
            for_each_xy(r, [&](int const x, int const y, bool const on_edge) noexcept {
                if (on_edge && can_remove_shared_wall(r, x, y)) {
                    tie_data(x, y) = floor_t;
                }
            });
        }
    }
private:
    template <typename Vector>
    static decltype(auto) data_at_(Vector&& v, int const x, int const y, sizeix const w) noexcept {
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

    struct data_t {
        std::vector<tile_id>    ids;
        std::vector<tile_flags> flags;
        std::vector<uint16_t>   tile_indicies;
        std::vector<uint16_t>   region_ids;
    } data_;
};

std::unique_ptr<level> make_level(random_state& rng, sizeix const width, sizeiy const height) {
    return std::make_unique<level_impl>(rng, width, height);
}

} //namespace boken