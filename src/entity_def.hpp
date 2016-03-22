#pragma once

#include "definition.hpp"
#include "types.hpp"
#include "hash.hpp"

namespace boken { class game_database; }

namespace boken {

enum class entity_property : uint32_t {
    invalid    = 0
  , temperment = djb2_hash_32c("temperment")
};

using entity_property_value = int32_t;
using entity_properties = property_set<entity_property, entity_property_value>;

struct entity_definition : basic_definition {
    using definition_type     = entity_id;
    using properties_type     = entity_properties;
    using property_type       = entity_properties::property_type;
    using property_value_type = entity_properties::value_type;

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
    int16_t health_per_level {1}; //TODO
};

bool has_property(game_database const& data, entity_id const& def, entity_property property) noexcept;
bool has_property(entity_definition const& def, entity_property property) noexcept;

entity_property_value property_value_or(game_database const& data, entity_id const& def, entity_property property, entity_property_value value) noexcept;
entity_property_value property_value_or(entity_definition const& def, entity_property property, entity_property_value value) noexcept;

} //namespace boken
