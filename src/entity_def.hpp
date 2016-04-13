#pragma once

#include "definition.hpp"
#include "types.hpp"

#include <cstdint>

namespace boken {

using entity_property_value = uint32_t;

struct entity_definition : basic_definition<entity_definition
                                          , entity_id
                                          , entity_property_id
                                          , entity_property_value>
{
    using basic_definition::basic_definition;

    int16_t health_per_level {1}; //TODO
};

} //namespace boken
