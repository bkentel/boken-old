#pragma once

#include "definition.hpp"
#include "types.hpp"
#include "hash.hpp"

namespace boken { class game_database; }

namespace boken {

using item_property_value = uint32_t;
using item_properties = property_set<item_property_id, item_property_value>;

struct item_definition : basic_definition {
    using definition_type     = item_id;
    using properties_type     = item_properties;
    using property_type       = item_properties::property_type;
    using property_value_type = item_properties::value_type;

    item_definition(basic_definition const& basic, item_id const id_)
      : basic_definition {basic}
      , id {id_}
    {
    }

    item_definition(item_id const id_ = item_id {})
      : basic_definition {"{null}", "{null}", "{null}", 0}
      , id {id_}
    {
    }

    item_id id;
    item_properties properties;
};

bool has_property(game_database const& data, item_id const& def, item_property_id property) noexcept;
bool has_property(item_definition const& def, item_property_id property) noexcept;

item_property_value property_value_or(game_database const& data, item_id const& def, item_property_id property, item_property_value value) noexcept;
item_property_value property_value_or(item_definition const& def, item_property_id property, item_property_value value) noexcept;


} //namespace boken
