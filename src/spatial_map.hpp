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

template <typename Container>
auto vector_to_range(Container&& c) noexcept {
    using iterator_cat = decltype(c.begin())::iterator_category;
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

    spatial_map(scalar_type const width, scalar_type const height)
      : width_  {width}
      , height_ {height}
    {
    }

    size_t size() const noexcept {
        return values_.size();
    }

    bool insert(point_type const p, value_type&& value) {
        auto const offset = find_offset_to_(p);
        if (offset >= 0) {
            return false;
        }

        insert_(p, std::move(value));
        return true;
    }

    bool insert_or_replace(point_type const p, value_type&& value) {
        auto const offset = find_offset_to_(p);
        if (offset >= 0) {
            *(positions_.begin() + offset) = p;
            *(keys_.begin() + offset) = KeyF {}(value);
            *(properties_.begin() + offset) = PropertyF {}(value);
            *(values_.begin() + offset) = std::move(value);
            return false;
        }

        insert_(p, std::move(value));
        return true;
    }

    bool erase(point_type const p) {
        return erase_(p);
    }

    bool erase(key_type const k) {
        return erase_(k);
    }

    value_type* find(point_type const p) noexcept {
        return find_(p);
    }

    value_type const* find(point_type const p) const noexcept {
        return find_(p);
    }

    value_type* find(key_type const k) noexcept {
        return find_(k);
    }

    value_type const* find(key_type const k) const noexcept {
        return find_(k);
    }

    auto positions_range() const noexcept {
        return vector_to_range(positions_);
    }

    auto properties_range() const noexcept {
        return vector_to_range(properties_);
    }
private:
    void insert_(point_type const p, value_type&& value) {
        positions_.push_back(p);
        keys_.push_back(KeyF {}(value));
        properties_.push_back(PropertyF {}(value));
        values_.push_back(std::move(value));
    }

    template <typename Key>
    bool erase_(Key const k) noexcept {
        auto const offset = find_offset_to_(k);
        if (offset < 0) {
            return false;
        }

        positions_.erase(positions_.begin() + offset);
        keys_.erase(keys_.begin() + offset);
        properties_.erase(properties_.begin() + offset);
        values_.erase(values_.begin() + offset);

        return true;
    }

    template <typename Key>
    value_type* find_(Key const k) noexcept {
        auto const offset = find_offset_to_(k);
        return offset < 0 ? nullptr : values_.data() + offset;
    }

    template <typename Key>
    value_type const* find_(Key const k) const noexcept {
        return const_cast<spatial_map*>(this)->find_(k);
    }

    ptrdiff_t find_offset_to_(point_type const p) const noexcept {
        return find_offset_to(positions_
          , [p](point_type const p0) noexcept { return p == p0; });
    }

    ptrdiff_t find_offset_to_(key_type const k) const noexcept {
        return find_offset_to(keys_
          , [k](key_type const k0) noexcept { return k == k0; });
    }
private:
    scalar_type width_;
    scalar_type height_;

    std::vector<point_type>    positions_;
    std::vector<key_type>      keys_;
    std::vector<property_type> properties_;
    std::vector<value_type>    values_;
};

} //namespace boken
