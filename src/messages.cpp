#include "messages.hpp"
#include "utility.hpp"
#include "item.hpp"
#include "entity.hpp"

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

} // namespace msg
} // namespace boken
