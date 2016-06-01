#pragma once

#include "config.hpp"
#include "context_fwd.hpp"

#include <string>

namespace boken {

//! Get the definition id string for the object.
//@{
string_view id_string(const_item_descriptor i) noexcept;
string_view id_string(const_entity_descriptor e) noexcept;
//@}

//! Get the simple "undecorated" name for an object.
//! For example, "chest" and not "chest [10]"
//@{
string_view name_of(const_context ctx, const_item_descriptor i) noexcept;
string_view name_of(const_context ctx, const_entity_descriptor e) noexcept;
//@}

//! Get the "decorated" name for an object.
//! For example, "chest [10]" and not "chest".
//@{
std::string name_of_decorated(const_context ctx, const_item_descriptor i);
std::string name_of_decorated(const_context ctx, const_entity_descriptor e);
//@}

} // namespace boken
