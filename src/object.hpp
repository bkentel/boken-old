#pragma once

#include "data.hpp"

#include <type_traits>
#include <utility>
#include <cstdint>

namespace boken {

template <typename Instance, typename Definition>
class object {
public:
    using instance_type       = Instance;
    using definition_type     = typename Definition::definition_type;
    using properties_type     = typename Definition::properties_type;
    using property_type       = typename Definition::property_type;
    using property_value_type = typename Definition::property_value_type;
    using property_pair_type  = std::pair<property_type, property_value_type>;

    object(instance_type const instance, definition_type const id) noexcept
      : instance_id_ {instance}
      , id_ {id}
    {
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Ids
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    instance_type   instance()   const noexcept { return instance_id_; }
    definition_type definition() const noexcept { return id_; }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Properties
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bool has_property(
        game_database const& data
      , property_type const  property
    ) const noexcept {
        if (properties_.has_property(property)) {
            return true;
        }

        auto const def = data.find(definition());
        return def && def->properties.has_property(property);
    }

    property_value_type property_value_or(
        game_database const&      data
      , property_type const       property
      , property_value_type const value
    ) const noexcept {
        if (properties_.has_property(property)) {
            return properties_.value_or(property, 0);
        }

        auto const def = data.find(definition());
        return def && def->properties.value_or(property, value);
    }

    bool add_or_update_property(
        property_type const       property
      , property_value_type const value
    ) {
        return properties_.add_or_update_property(property, value);
    }

    bool add_or_update_property(property_pair_type const property_pair) {
        return add_or_update_property(property_pair.first, property_pair.second);
    }

    template <typename InputIt>
    int add_or_update_properties(InputIt first, InputIt last) {
        return properties_.add_or_update_properties(first, last);
    }

    int add_or_update_properties(std::initializer_list<property_pair_type> properties) {
        return properties_.add_or_update_properties(properties);
    }

    bool remove_property(property_type const property) {
        return properties_.remove_property(property);
    }
private:
    instance_type   instance_id_ {0};
    definition_type id_          {0};
    properties_type properties_;
};

} //namespace boken
