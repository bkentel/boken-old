#pragma once

#include "config.hpp"
#include <string>
#include <cstdint>

namespace boken {

struct basic_definition {
    string_view source_name;
    uint32_t    source_line;
    std::string id_string;
    std::string name;
};

} //namespace boken
