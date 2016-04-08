#pragma once

#include "types.hpp"
#include "hash.hpp"

namespace boken {

//! common item properties
namespace iprop {

constexpr item_property_id weight             {djb2_hash_32c("weight")};
constexpr item_property_id stack_size         {djb2_hash_32c("stack_size")};
constexpr item_property_id current_stack_size {djb2_hash_32c("current_stack_size")};

} // namespace iprop

} // namespace boken
