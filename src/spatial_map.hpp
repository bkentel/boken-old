#pragma once

#include "math.hpp"

#include <vector>
#include <type_traits>
#include <algorithm>

namespace boken {

//! @returns an iterator to the first item in the container matching the
//! predicate. Otherwise returns the end iterator.
template <typename Container, typename Predicate>
auto find_iterator_to(Container&& c, Predicate pred) noexcept {
    return std::find_if(std::begin(c), std::end(c), pred);
}

//! @returns the offset from the start to the first item in the container
//! matching the predicate. Otherwise returns -1.
template <typename Container, typename Predicate>
ptrdiff_t find_offset_to(Container&& c, Predicate pred) noexcept {
    auto const first = std::begin(c);
    auto const last  = std::end(c);
    auto const it    = std::find_if(first, last, pred);

    return (it == last)
        ? ptrdiff_t {-1}
        : std::distance(first, it);
}

//! @returns a begin / end iterator pair for the container (vector).
template <typename Container>
auto vector_to_range(Container&& c) noexcept {
    using iterator_cat = typename decltype(c.begin())::iterator_category;
    static_assert(std::is_same<std::random_access_iterator_tag, iterator_cat>::value, "");
    return std::make_pair(c.data(), c.data() + c.size());
}

template <typename Value, typename KeyF, typename PropertyF, typename Scalar>
class spatial_map {
public:
    using value_type    = Value;
    using key_type      = std::result_of_t<KeyF(Value)>;
    using property_type = std::result_of_t<PropertyF(Value)>;
    using scalar_type   = Scalar;
    using point_type    = point2<Scalar>;

    static_assert(!std::is_void<key_type>::value, "");
    static_assert(!std::is_void<property_type>::value, "");

    spatial_map(
        scalar_type const width
      , scalar_type const height
      , KeyF      get_key      = KeyF {}
      , PropertyF get_property = PropertyF {}
    )
      : get_key_      {std::move(get_key)}
      , get_property_ {std::move(get_property)}
      , width_  {width}
      , height_ {height}
    {
    }

    size_t size() const noexcept {
        return values_.size();
    }

    std::pair<value_type*, bool> insert(point_type const p, value_type&& value) {
        auto const offset = find_offset_to_(p);
        if (offset >= 0) {
            return {values_.data() + offset, false};
        }

        return insert_(p, std::move(value));
    }

    std::pair<value_type*, bool> insert_or_replace(point_type const p, value_type&& value) {
        auto const offset = find_offset_to_(p);
        if (offset >= 0) {
            *(positions_.begin() + offset) = p;
            *(properties_.begin() + offset) = get_property_(value);
            *(values_.begin() + offset) = std::move(value);
            return {values_.data() + offset, false};
        }

        return insert_(p, std::move(value));
    }

    template <typename BinaryF>
    bool move_to_if(key_type const k, BinaryF f) noexcept {
        return move_to_if_(k, f);
    }

    bool move_to(key_type const k, point_type const p) noexcept {
        return move_to_(k, p);
    }

    template <typename BinaryF>
    bool move_to_if(point_type const p, BinaryF f) noexcept {
        return move_to_if_(p, f);
    }

    bool move_to(point_type const p, point_type const p0) noexcept {
        return move_to_(p, p0);
    }

    std::pair<key_type, bool> erase(point_type const p) {
        return erase_(p);
    }

    std::pair<key_type, bool> erase(key_type const k) {
        return erase_(k);
    }

    value_type* find(point_type const p) noexcept {
        auto const offset = find_offset_to_(p);
        return offset < 0
          ? nullptr
          : values_.data() + offset;
    }

    value_type const* find(point_type const p) const noexcept {
        return const_cast<spatial_map*>(this)->find(p);
    }

    std::pair<value_type*, point_type> find(key_type const k) noexcept {
        using pair_t = std::pair<value_type*, point_type>;
        auto const offset = find_offset_to_(k);
        return offset < 0
          ? pair_t {nullptr, {}}
          : pair_t {values_.data() + offset, *(positions_.data() + offset)};
    }

    std::pair<value_type const*, point_type>
    find(key_type const k) const noexcept {
        return const_cast<spatial_map*>(this)->find(k);
    }

    auto positions_range() const noexcept {
        return vector_to_range(positions_);
    }

    auto properties_range() const noexcept {
        return vector_to_range(properties_);
    }

    auto values_range() const noexcept {
        return vector_to_range(values_);
    }

    auto values_range() noexcept {
        return vector_to_range(values_);
    }
private:
    template <typename Key, typename BinaryF>
    bool move_to_if_(Key const k, BinaryF f) noexcept {
        auto const offset = find_offset_to_(k);
        if (offset < 0) {
            return false;
        }

        auto const result = f(*(values_.begin()    + offset)
                            , *(positions_.begin() + offset));

        if (!result.second) {
            return false;
        }

        *(positions_.begin() + offset) = result.first;
        return true;
    }

    template <typename Key>
    bool move_to_(Key const k, point_type const p) noexcept {
        return move_to_if(k
          , [p](auto&&) noexcept {
              return std::make_pair(true, p);
          });
    }

    std::pair<value_type*, bool> insert_(point_type const p, value_type&& value) {
        positions_.push_back(p);
        properties_.push_back(get_property_(value));
        values_.push_back(std::move(value));
        return {std::addressof(values_.back()), true};
    }

    template <typename Key>
    std::pair<key_type, bool> erase_(Key const k) noexcept {
        auto const offset = find_offset_to_(k);
        if (offset < 0) {
            return {key_type {}, false};
        }

        auto const result_key = get_key_(*(values_.begin() + offset));

        positions_.erase(positions_.begin() + offset);
        properties_.erase(properties_.begin() + offset);
        values_.erase(values_.begin() + offset);

        return {result_key, true};
    }

    ptrdiff_t find_offset_to_(point_type const p) const noexcept {
        return find_offset_to(positions_
          , [p](point_type const p0) noexcept { return p == p0; });
    }

    ptrdiff_t find_offset_to_(key_type const k) const noexcept {
        return find_offset_to(values_
          , [&](value_type const& v) noexcept { return k == get_key_(v); });
    }
private:
    KeyF      get_key_;
    PropertyF get_property_;

    std::vector<point_type>    positions_;
    std::vector<property_type> properties_;
    std::vector<value_type>    values_;

    scalar_type width_;
    scalar_type height_;
};

} //namespace boken
