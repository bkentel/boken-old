#pragma once

#include <type_traits>
#include <algorithm>
#include <vector>
#include <iterator>
#include <utility>
#include <initializer_list>

#include <cstddef>

namespace boken {

template <typename Property, typename Value>
class property_set {
    static_assert(std::is_standard_layout<Property>::value, "");
    static_assert(std::is_standard_layout<Value>::value, "");

    auto find_(Property const property) noexcept {
        return std::lower_bound(
            std::begin(values_), std::end(values_), property, compare_);
    }

    auto find_(Property const property) const noexcept {
        return const_cast<property_set*>(this)->find_(property);
    }

    auto has_property_(Property const property) noexcept {
        auto const it = find_(property);
        bool const ok = (it != std::end(values_)) && (it->first == property);
        return std::make_pair(it, ok);
    }

    auto has_property_(Property const property) const noexcept {
        return const_cast<property_set*>(this)->has_property_(property);
    }
public:
    using property_type = Property;
    using value_type    = Value;
    using pair_t        = std::pair<property_type, value_type>;

    size_t size()  const noexcept { return values_.size(); }
    bool   empty() const noexcept { return values_.empty(); }
    auto   begin() const noexcept { return values_.begin(); }
    auto   end()   const noexcept { return values_.end(); }

    std::pair<value_type, bool>
    get_property(Property const property) const noexcept {
        auto const pair  = has_property_(property);
        auto const value = pair.second ? pair.first->second : value_type {};
        return {value, pair.second};
    }

    bool has_property(Property const property) const noexcept {
        return has_property_(property).second;
    }

    Value value_or(Property const property, Value const value) const noexcept {
        auto const pair = has_property_(property);
        return pair.second ? pair.first->second : value;
    }

    // true if new
    // false if updated
    // find: O(log n); insert: O(n)
    bool add_or_update_property(Property const property, Value const value) {
        auto const pair = has_property_(property);
        if (!pair.second) {
            values_.insert(pair.first, {property, value});
        } else {
            pair.first->second = value;
        }

        return !pair.second;
    }

    bool add_or_update_property(pair_t const p) {
        return add_or_update_property(p.first, p.second);
    }

    // O(n*n)
    template <typename InputIt>
    int add_or_update_properties(InputIt first, InputIt last) {
        return std::accumulate(first, last, int {0}
          , [&](int const n, pair_t const p) {
                return n + (add_or_update_property(p) ? 1 : 0);
            });
    }

    int add_or_update_properties(std::initializer_list<pair_t> const properties) {
        return add_or_update_properties(
            std::begin(properties), std::end(properties));
    }

    bool remove_property(Property const property) noexcept {
        auto const first = std::begin(values_);
        auto const last  = std::end(values_);

        auto const it = std::remove_if(first, last
          , [&](pair_t const p) noexcept { return p.first == property; });

        bool const result = (it != last);
        if (result) {
            values_.erase(it);
        }

        return result;
    }

    void clear() {
        values_.clear();
    }
private:
    static bool compare_(pair_t const a, Property const b) noexcept {
        return a.first < b;
    }

    std::vector<pair_t> values_;
};

namespace detail {

template <typename Property, typename Value>
Value get_property_value_or(
    std::initializer_list<
        std::reference_wrapper<
            property_set<Property, Value> const>> const il
  , Property const property
  , Value    const fallback
) noexcept {
    for (property_set<Property, Value> const& properties : il) {
        auto const p = properties.get_property(property);
        if (p.second) {
            return p.first;
        }
    }

    return fallback;
}

} // namespace detail

template <typename Property, typename Value, typename... PropertySets>
Value get_property_value_or(
    Property const         property
  , Value const            fallback
  , PropertySets const&... property_sets
) noexcept {
    return detail::get_property_value_or(
        {std::cref(property_sets)...}, property, fallback);
}

} //namespace boken
