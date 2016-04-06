#pragma once

#include "hash.hpp"
#include "config.hpp" // string_view

#include <functional>
#include <memory>
#include <cstdint>

namespace boken { struct kb_event; }
namespace boken { struct text_input_event; }

namespace boken {

//! commands
enum class command_type : uint32_t {
    none          = djb2_hash_32c("none")
  , move_here     = djb2_hash_32c("move_here")
  , move_n        = djb2_hash_32c("move_n")
  , move_ne       = djb2_hash_32c("move_ne")
  , move_e        = djb2_hash_32c("move_e")
  , move_se       = djb2_hash_32c("move_se")
  , move_s        = djb2_hash_32c("move_s")
  , move_sw       = djb2_hash_32c("move_sw")
  , move_w        = djb2_hash_32c("move_w")
  , move_nw       = djb2_hash_32c("move_nw")
  , move_up       = djb2_hash_32c("move_up")
  , move_down     = djb2_hash_32c("move_down")
  , get_all_items = djb2_hash_32c("get_all_items")
  , reset_zoom    = djb2_hash_32c("reset_zoom")
  , reset_view    = djb2_hash_32c("reset_view")
  , cancel        = djb2_hash_32c("cancel")

  , toggle_show_inventory = djb2_hash_32c("toggle_show_inventory")

  , debug_toggle_regions = djb2_hash_32c("debug_toggle_regions")
  , debug_teleport_self  = djb2_hash_32c("debug_teleport_self")
};

template <typename Enum>
Enum string_to_enum(string_view str) noexcept;

string_view enum_to_string(command_type id) noexcept;

//! mapping between user input and game commands
class command_translator {
public:
    using command_handler_t = std::function<void (command_type, uint64_t)>;

    virtual ~command_translator();

    virtual void on_command(command_handler_t handler) = 0;
    virtual void translate(text_input_event const& event) const = 0;
    virtual void translate(kb_event const& event) const = 0;
};

std::unique_ptr<command_translator> make_command_translator();

} //namespace boken
