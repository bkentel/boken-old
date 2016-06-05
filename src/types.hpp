#pragma once

#include "id_fwd.hpp"

#include <memory>
#include <cstdint>

namespace boken { class world; }

namespace boken {
//===------------------------------------------------------------------------===
//                                  Tags
//===------------------------------------------------------------------------===
struct tag_id_entity          {};
struct tag_id_instance_entity {};
struct tag_id_property_entity {};
struct tag_id_item            {};
struct tag_id_instance_item   {};
struct tag_id_property_item   {};
struct tag_id_region          {};
struct tag_id_body_part       {};

using region_id = tagged_value<uint16_t, tag_id_region>;

using entity_property_value = uint32_t;
using item_property_value   = uint32_t;

//===------------------------------------------------------------------------===
//                              Custom deleters
//===------------------------------------------------------------------------===
namespace detail {

template <typename T>
class object_deleter {
public:
    using pointer = T;
    explicit object_deleter(world& w) noexcept : world_ {w} { }
    void operator()(pointer id) const noexcept;
private:
    std::reference_wrapper<world> world_;
};

} // namespace detail

using item_deleter   = detail::object_deleter<item_instance_id>;
using entity_deleter = detail::object_deleter<entity_instance_id>;
using unique_item    = std::unique_ptr<item_instance_id, item_deleter const&>;
using unique_entity  = std::unique_ptr<entity_instance_id, entity_deleter const&>;

item_deleter const& get_item_deleter(world const& w) noexcept;
entity_deleter const& get_entity_deleter(world const& w) noexcept;

} //namespace boken
