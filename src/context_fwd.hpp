#pragma once

#include "id_fwd.hpp"

namespace boken {

class world;
class game_database;

class entity;
class item;

struct entity_definition;
struct item_definition;

template <typename, typename> struct descriptor_base;
template <bool>               struct context_base;
template <bool>               struct level_location_base;

using context       = context_base<false>;
using const_context = context_base<true>;

using level_location       = level_location_base<false>;
using const_level_location = level_location_base<true>;

using item_descriptor       = descriptor_base<item,       item_definition>;
using const_item_descriptor = descriptor_base<item const, item_definition>;

using entity_descriptor       = descriptor_base<entity,       entity_definition>;
using const_entity_descriptor = descriptor_base<entity const, entity_definition>;

//=====--------------------------------------------------------------------=====
namespace detail {

enum class param_class {
    subject
  , object
  , at
  , from
  , to
};

template <typename, param_class> struct param_t;

} // namespace detail;

template <typename T>
using subject_t = detail::param_t<T, detail::param_class::subject>;

template <typename T>
using object_t = detail::param_t<T, detail::param_class::object>;

template <typename T>
using at_t = detail::param_t<T, detail::param_class::at>;

template <typename T>
using from_t = detail::param_t<T, detail::param_class::from>;

template <typename T>
using to_t = detail::param_t<T, detail::param_class::to>;

//=====--------------------------------------------------------------------=====
entity_id get_id(entity const&            e)   noexcept;
item_id   get_id(item const&              i)   noexcept;
entity_id get_id(entity_definition const& def) noexcept;
item_id   get_id(item_definition const&   def) noexcept;
entity_id get_id(const_entity_descriptor  e)   noexcept;
item_id   get_id(const_item_descriptor    i)   noexcept;

item   const& find(world const& w, item_instance_id   id) noexcept;
entity const& find(world const& w, entity_instance_id id) noexcept;
item&         find(world&       w, item_instance_id   id) noexcept;
entity&       find(world&       w, entity_instance_id id) noexcept;

item_definition   const* find(game_database const& db, item_id   id) noexcept;
entity_definition const* find(game_database const& db, entity_id id) noexcept;

} // namespace boken
