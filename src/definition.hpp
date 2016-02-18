#pragma once

#include "config.hpp"
#include <string>
#include <cstdint>

namespace boken {

struct basic_definition {
    basic_definition(std::string id_string_, std::string name_
                   , string_view const source_name_, uint32_t const source_line_)
      : id_string   {std::move(id_string_)}
      , name        {std::move(name_)}
      , source_name {source_name_}
      , source_line {source_line_}
    {
    }

    std::string id_string;
    std::string name;
    string_view source_name;
    uint32_t    source_line;
};

} //namespace boken
