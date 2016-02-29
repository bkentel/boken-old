#pragma once

#include "hash.hpp"
#include "config.hpp" // string_view

#include <functional>
#include <memory>
#include <cstdint>

namespace boken { struct kb_event; }

namespace boken {

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4307) // integral constant overflow
#endif

enum class command_type : uint32_t {
    none = djb2_hash_32c("none")

  , move_here = djb2_hash_32c("move_here")
  , move_n    = djb2_hash_32c("move_n")
  , move_ne   = djb2_hash_32c("move_ne")
  , move_e    = djb2_hash_32c("move_e")
  , move_se   = djb2_hash_32c("move_se")
  , move_s    = djb2_hash_32c("move_s")
  , move_sw   = djb2_hash_32c("move_sw")
  , move_w    = djb2_hash_32c("move_w")
  , move_nw   = djb2_hash_32c("move_nw")

  , reset_zoom = djb2_hash_32c("reset_zoom")
  , reset_view = djb2_hash_32c("reset_view")
};

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

template <typename Enum>
Enum string_to_enum(string_view str) noexcept;

string_view enum_to_string(command_type id) noexcept;

class command_translator {
public:
    using command_handler_t = std::function<void (command_type, uintptr_t)>;

    virtual ~command_translator();

    virtual void on_command(command_handler_t handler) = 0;
    virtual void translate(kb_event const& event) const = 0;
};

std::unique_ptr<command_translator> make_command_translator();

} //namespace boken
