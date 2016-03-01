#pragma once

#include "definition.hpp"
#include "types.hpp"
#include "hash.hpp"

#include <cstdint>
#include <vector>
#include <initializer_list>
#include <type_traits>

namespace boken {

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4307) // integral constant overflow
#endif

enum class entity_property : uint32_t {
    invalid    = 0
  , temperment = djb2_hash_32c("temperment")
};

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

template <typename Property, typename Value>
class property_set {
    static_assert(std::is_enum<Property>::value, "");
    static_assert(std::is_standard_layout<Value>::value, "");

    auto find_(Property const property) noexcept {
        return std::lower_bound(
            std::begin(values_), std::end(values_), property, compare {});
    }

    auto find_(Property const property) const noexcept {
        return const_cast<property_set*>(this)->find_(property);
    }
public:
    using pair_t = std::pair<Property, Value>;

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
private:


    struct compare {
        bool operator()(Property const& a, Property const& b) const noexcept {
            using type = std::underlying_type_t<Property>;
            return static_cast<type>(a) < static_cast<type>(b);
        }

        bool operator()(pair_t const& a, Property const& b) const noexcept {
            return (*this)(a.first, b);
        }

        bool operator()(pair_t const& a, pair_t const& b) const noexcept {
            return (*this)(a.first, b.first);
        }

        bool operator()(Property const& a, pair_t const& b) const noexcept {
            return (*this)(a, b.first);
        }
    };

    std::vector<pair_t> values_;
};

using entity_properties = property_set<entity_property, int32_t>;

struct entity_definition : basic_definition {
    entity_definition(basic_definition const& basic, entity_id const id_)
      : basic_definition {basic}
      , id {id_}
    {
    }

    entity_definition(entity_id const id_ = entity_id {0})
      : basic_definition {"{null}", "{null}", "{null}", 0}
      , id {id_}
    {
    }

    entity_id id;
    entity_properties properties;
};

} //namespace boken
