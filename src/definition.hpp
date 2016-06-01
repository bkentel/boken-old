#pragma once

#include "config.hpp"
#include "math_types.hpp"
#include "property_set.hpp"

#include <string>
#include <utility>

namespace boken {

template <typename Derived
        , typename Id
        , typename PropertyKey
        , typename PropertyValue>
struct basic_definition {
    using definition_id_t  = Id;
    using property_t       = PropertyKey;
    using property_value_t = PropertyValue;
    using properties_t     = property_set<PropertyKey, PropertyValue>;

    basic_definition() = default;

    basic_definition(std::string def_id_string, definition_id_t const def_id)
      : id        {def_id}
      , id_string {std::move(def_id_string)}
    {
    }

    properties_t    properties {};
    definition_id_t id         {};
    std::string     name       {"{null}"};
    std::string     id_string  {"{null}"};
};

} //namespace boken
