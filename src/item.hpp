#pragma once

#include "types.hpp"
#include "item_def.hpp"

namespace boken { class game_database; }

namespace boken {

class item {
public:
    item(item_instance_id const instance, item_id const id)
      : instance_id_ {instance}
      , id_ {id}
    {
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Ids
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    item_instance_id instance()   const noexcept { return instance_id_; }
    item_id          definition() const noexcept { return id_; }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Properties
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    using property_value = int32_t;
    using property_pair  = std::pair<item_property, int32_t>;

    bool has_property(game_database const& data, item_property p) const noexcept;

    property_value property_value_or(game_database const& data, item_property p, property_value value) const noexcept;

    bool add_or_update_property(item_property p, property_value value);
    bool add_or_update_property(property_pair const p) {
        return add_or_update_property(p.first, p.second);
    }

    template <typename InputIt>
    int add_or_update_properties(InputIt first, InputIt last) {
        return properties_.add_or_update_properties(first, last);
    }

    int add_or_update_properties(std::initializer_list<property_pair> properties);

    bool remove_property(item_property p);
private:
    item_instance_id instance_id_ {0};
    item_id          id_          {0};
    item_properties  properties_;
};

} //namespace boken
