#pragma once

#include "config.hpp"
#include <functional>

namespace boken { struct item_definition; }

namespace boken {

void load_item_definitions(
    std::function<void (item_definition const&)> const& on_read);

} //namespace boken
