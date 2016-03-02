#pragma once

#include "definition.hpp"
#include "types.hpp"
#include "hash.hpp"

#include <cstdint>

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

using entity_properties = property_set<entity_property, int32_t>;

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
};

} //namespace boken
