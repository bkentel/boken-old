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

enum class item_property : uint32_t {
    invalid    = 0
};

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

using item_properties = property_set<item_property, int32_t>;

struct item_definition : basic_definition {
    item_definition(basic_definition const& basic, item_id const id_)
      : basic_definition {basic}
      , id {id_}
    {
    }

    item_definition(item_id const id_ = item_id {0})
      : basic_definition {"{null}", "{null}", "{null}", 0}
      , id {id_}
    {
    }

    item_id id;
    item_properties properties;
};

} //namespace boken
