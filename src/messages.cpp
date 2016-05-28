#include "messages.hpp"
#include "format.hpp"
#include "forward_declarations.hpp"
#include "item_properties.hpp"
#include "entity_properties.hpp"

namespace boken {
namespace msg {

std::string drop_item(const_context const ctx,
                      ced const sub,
                      cid const obj,
                      ced const src,
                      cll const dst,
                      string_view const fail
) {
    static_string_buffer<128> buffer;

    if (fail.empty()) {
        buffer.append("%s drop the %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data());
    } else {
        buffer.append("%s can't drop the %s: %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , fail.data());
    }

    return buffer.to_string();
}

namespace detail {

std::string remove_item(const_context const ctx,
                        ced const sub,
                        cid const obj,
                        cid const src,
                        cll const dst,
                        string_view const fail
) {
    static_string_buffer<128> buffer;

    if (fail.empty()) {
        buffer.append("%s remove the %s from the %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , name_of_decorated(ctx, src).data());
    } else {
        buffer.append("%s can't remove the %s from the %s: %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , name_of_decorated(ctx, src).data()
          , fail.data());
    }

    return buffer.to_string();
}

std::string remove_item(const_context const ctx,
                        ced const sub,
                        cid const obj,
                        cid const src,
                        ced const dst,
                        string_view const fail
) {
    static_string_buffer<128> buffer;

    if (fail.empty()) {
        buffer.append("%s remove the %s from the %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , name_of_decorated(ctx, src).data());
    } else {
        buffer.append("%s can't remove the %s from the %s: %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , name_of_decorated(ctx, src).data()
          , fail.data());
    }

    return buffer.to_string();
}

}  // namespace detail

std::string insert_item(const_context const ctx,
                        ced const sub,
                        cid const obj,
                        ced const src,
                        cid const dst,
                        string_view const fail
) {
    static_string_buffer<128> buffer;

    if (fail.empty()) {
        buffer.append("%s put the %s in the %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , name_of_decorated(ctx, dst).data());
    } else {
        buffer.append("%s can't put the %s in the %s: %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , name_of_decorated(ctx, dst).data()
          , fail.data());
    }

    return buffer.to_string();
}

std::string pickup_item(const_context const ctx,
                        ced const sub,
                        cid const obj,
                        cll const src,
                        ced const dst,
                        string_view const fail
) {
    static_string_buffer<128> buffer;

    if (fail.empty()) {
        buffer.append("%s pick up the %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data());
    } else {
        buffer.append("%s can't pick up the %s: %s."
          , name_of_decorated(ctx, sub).data()
          , name_of_decorated(ctx, obj).data()
          , fail.data());
    }

    return buffer.to_string();
}

bool debug_item_info(string_buffer_base& buffer, const_context const ctx, cid const itm) {
    return buffer.append(
        " Instance  : %0#10x\n"
        " Definition: %0#10x (%s)\n"
        " Name      : %s\n"
      , value_cast(get_instance(itm.obj))
      , value_cast(get_id(itm.obj)), id_string(itm).data()
      , name_of(ctx, itm).data());
}

bool debug_entity_info(string_buffer_base& buffer, const_context const ctx, ced const ent) {
    return buffer.append(
        "Entity:\n"
        " Instance  : %0#10x\n"
        " Definition: %0#10x (%s)\n"
        " Name      : %s\n"
      , value_cast(get_instance(ent.obj))
      , value_cast(get_id(ent.obj)), id_string(ent).data()
      , name_of(ctx, ent).data());
}

std::string debug_item_info(const_context const ctx, cid const itm) {
    static_string_buffer<128> buffer;
    debug_item_info(buffer, ctx, itm);
    return buffer.to_string();
}

std::string debug_entity_info(const_context const ctx, ced const ent) {
    static_string_buffer<128> buffer;
    debug_entity_info(buffer, ctx, ent);
    return buffer.to_string();
}

bool view_item_info(string_buffer_base& buffer, const_context const ctx, cid const itm) {
    return buffer.append("%s", name_of_decorated(ctx, itm).data());
}

bool view_entity_info(string_buffer_base& buffer, const_context const ctx, ced const ent) {
    return buffer.append("%s", name_of(ctx, ent).data());
}

std::string view_item_info(const_context const ctx, cid const itm) {
    static_string_buffer<128> buffer;
    view_item_info(buffer, ctx, itm);
    return buffer.to_string();
}

std::string view_entity_info(const_context const ctx, ced const ent) {
    static_string_buffer<128> buffer;
    view_entity_info(buffer, ctx, ent);
    return buffer.to_string();
}

} // namespace msg
} // namespace boken
