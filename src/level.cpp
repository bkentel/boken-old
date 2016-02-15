#include "level.hpp"
#include "bsp_generator.hpp"
#include "random.hpp"
#include <vector>

namespace boken {

class level_impl : public level {
public:
    level_impl(sizeix const width, sizeiy const height)
      : width_  {width}
      , height_ {height}
    {
        bsp_generator::param_t p;
        p.width  = sizeix {width};
        p.height = sizeiy {height};
        bsp_gen_ = make_bsp_generator(p);
    }

    item const* find(item_instance_id const id) const noexcept final override {
        return nullptr;
    }

    entity const* find(entity_instance_id const id) const noexcept final override {
        return nullptr;
    }

    tile_view at(int const x, int const y) const noexcept final override {
        if (!check_bounds_(x, y)) {
            static tile_id    const dummy_id         {};
            static tile_flags const dummy_flags      {};
            static uint16_t   const dummy_tile_index {};
            static tile_data* const dummy_data       {};

            return {
                dummy_id
              , dummy_flags
              , dummy_tile_index
              , dummy_data
            };
        }

        return {
            data_at_(data_.ids          , x, y, width_)
          , data_at_(data_.flags        , x, y, width_)
          , data_at_(data_.tile_indicies, x, y, width_)
          , nullptr
        };
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
    } data_;
};

std::unique_ptr<level> make_level(sizeix const width, sizeiy const height) {
    return std::make_unique<level_impl>(width, height);
}

} //namespace boken