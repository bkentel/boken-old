#include "serialize.hpp"
#include "hash.hpp"
#include "item_def.hpp"

#include "scope_guard.hpp"

#include <rapidjson/reader.h>
#include <rapidjson/filereadstream.h>

#include <string>
#include <cstdint>

namespace boken {

class item_definition_handler {
public:
    enum class type {
        none
      , null, boolean, i32, u32, i64, u64, float_p, string
      , obj_start, obj_key, obj_end
      , arr_start, arr_end
    };

    enum class state {
        start
      ,   type, type_value
      ,   data, data_start
      ,     item_id_or_end
      ,     item_id, item_start
      ,       item_name, item_name_value
      ,       item_properties, item_properties_start
      ,         item_property_name_or_end
      ,         item_property_name, item_property_value
      ,       item_properties_end
      ,     item_end
      ,   data_end
      , end
    };

    item_definition_handler() = default;

    bool run();
public:
    bool Null() noexcept {
        last_type_ = type::null;
        return run();
    }

    bool Bool(bool const b) noexcept {
        last_type_ = type::boolean;
        last_bool_ = b;
        return run();
    }

    bool RawNumber(const char* const s, size_t const len, bool const copy) {
        return true;
    }

    bool Int(int const i) {
        last_type_ = type::i32;
        last_i32_ = i;
        return run();
    }

    bool Uint(unsigned const i) {
        last_type_ = type::u32;
        last_u32_ = i;
        return run();
    }

    bool Int64(int64_t const i) {
        last_type_ = type::i64;
        last_i64_ = i;
        return run();
    }

    bool Uint64(uint64_t const i) {
        last_type_ = type::u64;
        last_u64_ = i;
        return run();
    }

    bool Double(double const d) {
        last_type_ = type::float_p;
        last_double_ = d;
        return run();
    }

    bool String(const char* const s, size_t const len, bool const copy) {
        last_type_ = type::string;
        last_string_.assign(s, len);
        last_string_hash_ = djb2_hash_32(s, s + len);
        return run();
    }

    bool StartObject() {
        last_type_ = type::obj_start;
        return run();
    }

    bool EndObject(size_t const n) {
        last_type_ = type::obj_end;
        return run();
    }

    bool Key(const char* const s, size_t const len, bool const copy) {
        last_type_ = type::obj_key;
        last_string_.assign(s, len);
        last_string_hash_ = djb2_hash_32(s, s + len);
        return run();
    }

    bool StartArray() {
        last_type_ = type::arr_start;
        return run();
    }

    bool EndArray(size_t const n) {
        last_type_ = type::arr_end;
        return run();
    }
private:
    item_definition def_;

    std::string last_property_name_;

    type  last_type_ = type::none;
    state state_     = state::start;

    uint32_t    last_string_hash_ {};
    std::string last_string_ {};
    double      last_double_ {};
    int64_t     last_i64_    {};
    uint64_t    last_u64_    {};
    unsigned    last_u32_    {};
    int         last_i32_    {};
    bool        last_bool_   {};
private:

    bool require(type const expected_type, state const next_state) noexcept {
        return (last_type_ == expected_type) && ((state_ = next_state), true);
    }

    template <typename T>
    bool require(type const expected_type, T const& value, T const& expected, state const next_state) noexcept {
        return (value == expected) && require(expected_type, next_state);
    }

    template <typename Action>
    bool require(type const expected_type, state const next_state, Action a) {
        return require(expected_type, next_state) && (a(), true);
    }

    template <typename Action, typename T>
    bool require(type const expected_type, T const& value, T const& expected, state const next_state, Action a) {
        return require(expected_type, value, expected, next_state) && (a(), true);
    }
};

bool item_definition_handler::run() {
    switch (state_) {
    case state::start:
        return require(type::obj_start, state::type);
    case state::type:
        return require(type::obj_key
          , last_string_hash_, djb2_hash_32c("type"), state::type_value);
    case state::type_value:
        return require(type::string
          , last_string_hash_, djb2_hash_32c("items"), state::data);
    case state::data:
        return require(type::obj_key
          , last_string_hash_, djb2_hash_32c("data"), state::data_start);
    case state::data_start:
        return require(type::obj_start, state::item_id_or_end);
    case state::item_id_or_end:
        if (last_type_ == type::obj_key) {
            state_ = state::item_id;
            return run();
        } else if (last_type_ == type::obj_end) {
            state_ = state::data_end;
            return run();
        }

        return false;
    case state::item_id:
        return require(type::obj_key, state::item_start, [&] {
            def_.id_string = last_string_;
            def_.id = item_id {last_string_hash_};
        });
    case state::item_start:
        return require(type::obj_start, state::item_name);
    case state::item_name:
        return require(type::obj_key
          , last_string_hash_, djb2_hash_32c("name"), state::item_name_value);
    case state::item_name_value:
        return require(type::string, state::item_properties, [&] {
            def_.name = last_string_;
        });
    case state::item_properties:
        return require(type::obj_key
          , last_string_hash_, djb2_hash_32c("properties"), state::item_properties_start);
    case state::item_properties_start:
        return require(type::obj_start, state::item_property_name_or_end);
    case state::item_property_name_or_end:
        if (last_type_ == type::obj_key) {
            state_ = state::item_property_name;
            return run();
        } else if (last_type_ == type::obj_end) {
            state_ = state::item_properties_end;
            return run();
        }

        return false;
    case state::item_property_name:
        return require(type::obj_key, state::item_property_value, [&] {
            last_property_name_ = last_string_;
        });
    case state::item_property_value:
        switch (last_type_) {
        case type::null:
            break;
        case type::boolean:
            break;
        case type::i32:
            break;
        case type::u32:
            break;
        case type::i64:
            break;
        case type::u64:
            break;
        case type::float_p:
            break;
        case type::string:
            break;
        case type::none:
        case type::obj_start:
        case type::obj_key:
        case type::obj_end:
        case type::arr_start:
        case type::arr_end:
        default:
            return false;
        }

        state_ = state::item_property_name_or_end;
        return true;
    case state::item_properties_end:
        return require(type::obj_end, state::item_end);
    case state::item_end:
        return require(type::obj_end, state::item_id_or_end);
    case state::data_end:
        return require(type::obj_end, state::end);
    case state::end:
        return require(type::obj_end, state::start);
    default:
        break;
    }

    return false;
}

void load_item_definitions(
    std::function<void(item_definition const&)> const& on_read
) {
    constexpr size_t buffer_size = 65536;

    item_definition_handler handler;

    rapidjson::Reader reader;
    char buffer[buffer_size];

    auto const handle = fopen("./data/items.dat", "rb");
    auto const on_exit = BK_SCOPE_EXIT {
        fclose(handle);
    };

    rapidjson::FileReadStream in {handle, buffer, buffer_size};

    reader.Parse(in, handler);
}

} //namespace boken
