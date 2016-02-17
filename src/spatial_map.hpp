#pragma once

#include "math.hpp"

#include <vector>
#include <type_traits>
#include <algorithm>
#include <numeric>

namespace boken {

template <typename T, typename Point = point2i>
class spatial_map {
    static_assert(std::is_pod<T>::value, "");
public:
    spatial_map(int const width, int const height)
      : width_  {width}
      , height_ {height}
    {
    }

    size_t size() const noexcept {
        return items_.size();
    }

    template <typename InputIt>
    void insert(InputIt first, InputIt last) {
        std::transform(first, last, back_inserter(items_), [&](auto const& i) {
            return std::make_pair(id_of(i), position_of(i));
        });
    }

    template <typename Transform>
    void update_all(Transform trans) {
        for (auto& i : items_) {
            i.second = trans(i.first, i.second);
        }
    }

    T const* at(Point const p) const noexcept {
        auto const it = std::find_if(begin(items_), end(items_)
          , [p](auto const& i) noexcept { return i.second == p; });

        return it == end(items_) ? nullptr : &it->first;
    }

    template <typename U, typename BinaryF>
    int near(Point const p, U const distance, BinaryF f) const {
        int count = 0;

        auto const d2 = size_type<U>(distance * distance);
        for (auto const& i : items_) {
            if (distance2(p, i.second) < d2) {
                ++count;
                f(i.first, i.second);
            }
        }

        return count;
    }
private:
    int width_;
    int height_;
    std::vector<std::pair<T, Point>> items_;
};

} //namespace boken
