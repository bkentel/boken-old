#pragma once

#include "definition.hpp"
#include "types.hpp"

#include <cstdint>

namespace boken {

using item_property_value = uint32_t;

struct item_definition : basic_definition<item_definition
                                        , item_id
                                        , item_property_id
                                        , item_property_value>
{
    using basic_definition::basic_definition;
};

} //namespace boken
