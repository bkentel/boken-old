#pragma once

#include "types.hpp"
#include "entity_def.hpp"

namespace boken { class game_database; }

namespace boken {

class entity {
public:
    entity(entity_instance_id const instance, entity_id const id)
      : instance_id_ {instance}
      , id_ {id}
    {
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Ids
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    entity_instance_id instance()   const noexcept { return instance_id_; }
    entity_id          definition() const noexcept { return id_; }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Properties
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    using property_value = int32_t;
    using property_pair  = std::pair<entity_property, int32_t>;

    bool has_property(game_database const& data, entity_property p) const noexcept;

    property_value property_value_or(game_database const& data, entity_property p, property_value value) const noexcept;

    bool add_or_update_property(entity_property p, property_value value);
    bool add_or_update_property(property_pair const p) {
        return add_or_update_property(p.first, p.second);
    }

    template <typename InputIt>
    int add_or_update_properties(InputIt first, InputIt last) {
        return properties_.add_or_update_properties(first, last);
    }

    int add_or_update_properties(std::initializer_list<property_pair> properties);

    bool remove_property(entity_property p);
private:
    entity_instance_id instance_id_ {0};
    entity_id          id_          {0};
    entity_properties  properties_;
};

} //namespace boken
