#pragma once

#include "config.hpp"
#include <algorithm>
#include <numeric>
#include <vector>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <cstdint>

namespace boken {

template <typename Property, typename Value>
class property_set {
    static_assert(std::is_standard_layout<Property>::value, "");
    static_assert(std::is_standard_layout<Value>::value, "");

    auto find_(Property const property) noexcept {
        return std::lower_bound(
            std::begin(values_), std::end(values_), property
          , [](pair_t const& a, Property const b) noexcept {
                return a.first < b;
            });
    }

    auto find_(Property const property) const noexcept {
        return const_cast<property_set*>(this)->find_(property);
    }
public:
    using property_type = Property;
    using value_type    = Value;
    using pair_t        = std::pair<property_type, value_type>;

    size_t size()  const noexcept { return values_.size(); }
    bool   empty() const noexcept { return values_.empty(); }
    auto   begin() const noexcept { return values_.begin(); }
    auto   end()   const noexcept { return values_.end(); }

    bool has_property(Property const property) const noexcept {
        return find_(property) != std::end(values_);
    }

    Value value_or(Property const property, Value const value) const noexcept {
        auto const it = find_(property);
        return (it != std::end(values_)) && (it->first == property)
          ? it->second
          : value;
    }

    // true if new
    // false if updated
    // find: O(log n); insert: O(n)
    bool add_or_update_property(Property const property, Value const value) {
        auto const it = find_(property);
        if (it == std::end(values_) || it->first != property) {
            values_.insert(it, {property, value});
            return true;
        }

        it->second = value;
        return false;
    }

    bool add_or_update_property(pair_t const& p) {
        return add_or_update_property(p.first, p.second);
    }

    // O(n*n)
    template <typename InputIt>
    int add_or_update_properties(InputIt first, InputIt last) {
        return std::accumulate(first, last, int {0}
          , [&](int const n, pair_t const& p) {
                return n + (add_or_update_property(p) ? 1 : 0);
            });
    }

    int add_or_update_properties(std::initializer_list<pair_t> properties) {
        return add_or_update_properties(
            std::begin(properties), std::end(properties));
    }

    bool remove_property(Property const property) {
        auto const it = std::remove_if(std::begin(values_), std::end(values_)
          , [&](pair_t const& p) noexcept {
                return p.first == property;
          });

        return it != std::end(values_) && (values_.erase(it), true);
    }

    void clear() {
        values_.clear();
    }
private:
    std::vector<pair_t> values_;
};

struct basic_definition {
    basic_definition(std::string id_string_, std::string name_
                   , string_view const source_name_, uint32_t const source_line_)
      : id_string   {std::move(id_string_)}
      , name        {std::move(name_)}
      , source_name {source_name_}
      , source_line {source_line_}
    {
    }

    std::string id_string;
    std::string name;
    string_view source_name;
    uint32_t    source_line;
};

} //namespace boken
