// currently the implementations for item and entity parsing are nearly
// identical, but as the diverge so too will the parsers.

#include "serialize.hpp"
#include "hash.hpp"
#include "item_def.hpp"
#include "entity_def.hpp"

#include "scope_guard.hpp"

#include <bkassert/assert.hpp>

#include <rapidjson/reader.h>
#include <rapidjson/filereadstream.h>

#include <string>
#include <cstdint>

namespace boken {

uint32_t to_property(nullptr_t) noexcept {
    return 0u;
}

uint32_t to_property(bool const n) noexcept {
    return n ? 1u : 0u;
}

uint32_t to_property(int32_t const n) noexcept {
    return static_cast<uint32_t>(n);
}

uint32_t to_property(uint32_t const n) noexcept {
    return n;
}

uint32_t to_property(double const n) noexcept {
    // 16.16 fixed point
    return static_cast<uint32_t>(n * (1 << 16u));
}

uint32_t to_property(string_view const n) noexcept {
    return djb2_hash_32(n.begin(), n.end());
}

namespace {

enum class element_type {
    none
  , null, boolean, i32, u32, i64, u64, float_p, string
  , obj_start, obj_key, obj_end
  , arr_start, arr_end
};

} // namespace

template <typename Derived, typename StateType>
class definition_handler_base {
public:
    using state_type = StateType;

    definition_handler_base() = default;

    bool run() {
        return static_cast<Derived*>(this)->run();
    }

    state_type current_state() const noexcept {
        return static_cast<Derived*>(this)->current_state();
    }

    void set_current_state(state_type const state) noexcept {
        return static_cast<Derived*>(this)->set_current_state(state);
    }

    element_type current_type() const noexcept {
        return last_type_;
    }

    bool transition(
        element_type const expected_type
      , state_type   const next_state
    ) noexcept {
        if (last_type_ != expected_type) {
            return false;
        }

        set_current_state(next_state);
        return true;
    }

    template <typename T>
    bool transition(
        element_type const  expected_type
      , T            const& value
      , T            const& expected
      , state_type   const  next_state
    ) noexcept {
        return (value == expected) && transition(expected_type, next_state);
    }

    template <typename Action>
    bool transition(
        element_type const expected_type
      , state_type   const next_state
      , Action             a
    ) {
        if (!transition(expected_type, next_state)) {
            return false;
        }

        a();
        return true;
    }

    template <typename Action, typename T>
    bool transition(
        element_type const  expected_type
      , T            const& value
      , T            const& expected
      , state_type   const  next_state
      , Action              a
    ) {
        if (!transition(expected_type, value, expected, next_state)) {
            return false;
        }

        a();
        return true;
    }

    std::pair<serialize_data_type, uint32_t>
    to_property() noexcept {
        using st = serialize_data_type;

        switch (last_type_) {
        case element_type::null:
            return std::make_pair(st::null, boken::to_property(nullptr));
        case element_type::boolean:
            return std::make_pair(st::boolean, boken::to_property(last_bool_));
        case element_type::i32:
            return std::make_pair(st::i32, boken::to_property(last_i32_));
        case element_type::u32:
            return std::make_pair(st::u32, boken::to_property(last_u32_));
        case element_type::float_p:
            return std::make_pair(st::float_p, boken::to_property(last_double_));
        case element_type::string:
            return std::make_pair(st::string, last_string_hash_);
        case element_type::i64:       BK_ATTRIBUTE_FALLTHROUGH;
        case element_type::u64:       BK_ATTRIBUTE_FALLTHROUGH;
        case element_type::none:      BK_ATTRIBUTE_FALLTHROUGH;
        case element_type::obj_start: BK_ATTRIBUTE_FALLTHROUGH;
        case element_type::obj_key:   BK_ATTRIBUTE_FALLTHROUGH;
        case element_type::obj_end:   BK_ATTRIBUTE_FALLTHROUGH;
        case element_type::arr_start: BK_ATTRIBUTE_FALLTHROUGH;
        case element_type::arr_end:   BK_ATTRIBUTE_FALLTHROUGH;
        default:
            BK_ASSERT(false);
            break;
        }

        return std::make_pair(st::null, boken::to_property(nullptr));
    }
public:
    bool Null() noexcept {
        last_type_ = element_type::null;
        return run();
    }

    bool Bool(bool const b) noexcept {
        last_type_ = element_type::boolean;
        last_bool_ = b;
        return run();
    }

    bool RawNumber(const char*, size_t, bool) noexcept {
        return true;
    }

    bool Int(int const i) noexcept {
        last_type_ = element_type::i32;
        last_i32_ = i;
        return run();
    }

    bool Uint(unsigned const i) noexcept {
        last_type_ = element_type::u32;
        last_u32_ = i;
        return run();
    }

    bool Int64(int64_t const i) noexcept {
        last_type_ = element_type::i64;
        last_i64_ = i;
        return run();
    }

    bool Uint64(uint64_t const i) noexcept {
        last_type_ = element_type::u64;
        last_u64_ = i;
        return run();
    }

    bool Double(double const d) noexcept {
        last_type_ = element_type::float_p;
        last_double_ = d;
        return run();
    }

    bool String(const char* const s, size_t const len, bool) {
        last_type_ = element_type::string;
        last_string_.assign(s, len);
        last_string_hash_ = djb2_hash_32(s, s + len);
        return run();
    }

    bool StartObject() noexcept {
        last_type_ = element_type::obj_start;
        return run();
    }

    bool EndObject(size_t) noexcept {
        last_type_ = element_type::obj_end;
        return run();
    }

    bool Key(const char* const s, size_t const len, bool) {
        last_type_ = element_type::obj_key;
        last_string_.assign(s, len);
        last_string_hash_ = djb2_hash_32(s, s + len);
        return run();
    }

    bool StartArray() noexcept {
        last_type_ = element_type::arr_start;
        return run();
    }

    bool EndArray(size_t) noexcept {
        last_type_ = element_type::arr_end;
        return run();
    }
protected:
    element_type last_type_        {element_type::none};
    uint32_t     last_string_hash_ {};
    std::string  last_string_      {};
    double       last_double_      {};
    int64_t      last_i64_         {};
    uint64_t     last_u64_         {};
    unsigned     last_u32_         {};
    int          last_i32_         {};
    bool         last_bool_        {};
};

namespace {

enum class item_definition_handler_state {
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

} // namespace

class item_definition_handler
    : public definition_handler_base<item_definition_handler
                                   , item_definition_handler_state>
{
public:
    using state_type = definition_handler_base::state_type;

    item_definition_handler(
        on_finish_item_definition const& on_finish
      , on_add_new_item_property  const& on_property
    ) : on_finish_   {on_finish}
      , on_property_ {on_property}
    {
    }

    state_type current_state() const noexcept {
        return state_;
    }

    void set_current_state(state_type const state) noexcept {
        state_ = state;
    }

    bool run();
private:
    bool add_property() {
        auto const p = to_property();

        auto const result = on_property_(
            last_property_name_
          , last_property_name_hash_
          , p.first
          , p.second);

        if (!result) {
            return false;
        }

        def_.properties.add_or_update_property(
            item_property_id {last_property_name_hash_}, p.second);

        return true;
    }
private:
    on_finish_item_definition const& on_finish_;
    on_add_new_item_property  const& on_property_;

    item_definition def_;

    std::string last_property_name_      {};
    uint32_t    last_property_name_hash_ {};

    state_type state_ {state_type::start};

};

bool item_definition_handler::run() {
    using st = state_type;
    using et = element_type;

    for (;;) switch (state_) {
    case st::start:
        return transition(et::obj_start
                        , st::type);
    case st::type:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("type")
                        , st::type_value);
    case st::type_value:
        return transition(et::string
                        , last_string_hash_
                        , djb2_hash_32c("items")
                        , st::data);
    case st::data:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("data")
                        , st::data_start);
    case st::data_start:
        return transition(et::obj_start
                        , st::item_id_or_end);
    case st::item_id_or_end:
        if (last_type_ == et::obj_key) {
            state_ = st::item_id;
            continue;
        } else if (last_type_ == et::obj_end) {
            state_ = st::data_end;
            continue;
        }

        return false;
    case st::item_id:
        return transition(et::obj_key, st::item_start, [&] {
            def_.id_string = last_string_;
            def_.id = item_id {last_string_hash_};
        });
    case st::item_start:
        return transition(et::obj_start
                        , st::item_name);
    case st::item_name:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("name")
                        , st::item_name_value);
    case st::item_name_value:
        return transition(et::string
                        , st::item_properties
                        , [&] {
                              def_.name = last_string_;
                          });
    case st::item_properties:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("properties")
                        , st::item_properties_start);
    case st::item_properties_start:
        return transition(et::obj_start
                        , st::item_property_name_or_end);
    case st::item_property_name_or_end:
        if (last_type_ == et::obj_key) {
            state_ = st::item_property_name;
            continue;
        } else if (last_type_ == et::obj_end) {
            state_ = st::item_properties_end;
            continue;
        }

        return false;
    case st::item_property_name:
        return transition(et::obj_key, st::item_property_value, [&] {
            last_property_name_      = last_string_;
            last_property_name_hash_ = last_string_hash_;
        });
    case st::item_property_value:
        if (!add_property()) {
            return false;
        }

        state_ = st::item_property_name_or_end;
        return true;
    case st::item_properties_end:
        return transition(et::obj_end
                        , st::item_end);
    case st::item_end:
        return transition(et::obj_end
                        , st::item_id_or_end
                        , [&] {
                              on_finish_(def_);
                              def_.id = item_id {};
                              def_.id_string.clear();
                              def_.name.clear();
                              def_.properties.clear();
                        });
    case st::data_end:
        return transition(et::obj_end
                        , st::end);
    case st::end:
        return transition(et::obj_end
                        , st::start);
    default:
        BK_ASSERT(false);
        break;
    }

    return false;
}

namespace {

enum class entity_definition_handler_state {
    start
  ,   type, type_value
  ,   data, data_start
  ,     entity_id_or_end
  ,     entity_id, entity_start
  ,       entity_name, entity_name_value
  ,       entity_properties, entity_properties_start
  ,         entity_property_name_or_end
  ,         entity_property_name, entity_property_value
  ,       entity_properties_end
  ,     entity_end
  ,   data_end
  , end
};

} // namespace

class entity_definition_handler
    : public definition_handler_base<entity_definition_handler
                                   , entity_definition_handler_state>
{
public:
    using state_type = definition_handler_base::state_type;

    entity_definition_handler(
        on_finish_entity_definition const& on_finish
      , on_add_new_entity_property  const& on_property
    ) : on_finish_   {on_finish}
      , on_property_ {on_property}
    {
    }

    state_type current_state() const noexcept {
        return state_;
    }

    void set_current_state(state_type const state) noexcept {
        state_ = state;
    }

    bool run();
private:
    bool add_property() {
        auto const p = to_property();

        auto const result = on_property_(
            last_property_name_
          , last_property_name_hash_
          , p.first
          , p.second);

        if (!result) {
            return false;
        }

        def_.properties.add_or_update_property(
            item_property_id {last_property_name_hash_}, p.second);

        return true;
    }
private:
    on_finish_entity_definition const& on_finish_;
    on_add_new_entity_property  const& on_property_;

    entity_definition def_;

    std::string last_property_name_      {};
    uint32_t    last_property_name_hash_ {};

    state_type state_ {state_type::start};
private:

};

bool entity_definition_handler::run() {
    using st = state_type;
    using et = element_type;

    for (;;) switch (state_) {
    case st::start:
        return transition(et::obj_start
                        , st::type);
    case st::type:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("type")
                        , st::type_value);
    case st::type_value:
        return transition(et::string
                        , last_string_hash_
                        , djb2_hash_32c("entities")
                        , st::data);
    case st::data:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("data")
                        , st::data_start);
    case st::data_start:
        return transition(et::obj_start
                        , st::entity_id_or_end);
    case st::entity_id_or_end:
        if (last_type_ == et::obj_key) {
            state_ = st::entity_id;
            continue;
        } else if (last_type_ == et::obj_end) {
            state_ = st::data_end;
            continue;
        }

        return false;
    case st::entity_id:
        return transition(et::obj_key, st::entity_start, [&] {
            def_.id_string = last_string_;
            def_.id = entity_id {last_string_hash_};
        });
    case st::entity_start:
        return transition(et::obj_start
                        , st::entity_name);
    case st::entity_name:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("name")
                        , st::entity_name_value);
    case st::entity_name_value:
        return transition(et::string
                        , st::entity_properties
                        , [&] {
                              def_.name = last_string_;
                          });
    case st::entity_properties:
        return transition(et::obj_key
                        , last_string_hash_
                        , djb2_hash_32c("properties")
                        , st::entity_properties_start);
    case st::entity_properties_start:
        return transition(et::obj_start
                        , st::entity_property_name_or_end);
    case st::entity_property_name_or_end:
        if (last_type_ == et::obj_key) {
            state_ = st::entity_property_name;
            continue;
        } else if (last_type_ == et::obj_end) {
            state_ = st::entity_properties_end;
            continue;
        }

        return false;
    case st::entity_property_name:
        return transition(et::obj_key, st::entity_property_value, [&] {
            last_property_name_      = last_string_;
            last_property_name_hash_ = last_string_hash_;
        });
    case st::entity_property_value:
        if (!add_property()) {
            return false;
        }

        state_ = st::entity_property_name_or_end;
        return true;
    case st::entity_properties_end:
        return transition(et::obj_end
                        , st::entity_end);
    case st::entity_end:
        return transition(et::obj_end
                        , st::entity_id_or_end
                        , [&] {
                              on_finish_(def_);
                              def_.id = entity_id {};
                              def_.id_string.clear();
                              def_.name.clear();
                              def_.properties.clear();
                        });
    case st::data_end:
        return transition(et::obj_end
                        , st::end);
    case st::end:
        return transition(et::obj_end
                        , st::start);
    default:
        BK_ASSERT(false);
        break;
    }

    return false;
}

namespace {

template <typename Handler, typename Finish, typename Property>
void impl_load_definitions_(
    string_view const filename
  , Finish   const& on_finish
  , Property const& on_property
) {
    constexpr size_t buffer_size = 65536;

    Handler handler {on_finish, on_property};

    rapidjson::Reader reader {nullptr};
    char buffer[buffer_size];

    auto const handle = fopen(filename.data(), "rb");
    if (!handle) {
        BK_ASSERT(false); // TODO error handing
    }

    auto const on_exit = BK_SCOPE_EXIT {
        fclose(handle);
    };

    rapidjson::FileReadStream in {handle, buffer, buffer_size};

    auto const result = reader.Parse(in, handler);
    if (!result) {
        BK_ASSERT(false); //TODO parsing error
    }
}

} // namespace

void load_item_definitions(
    on_finish_item_definition const& on_finish
  , on_add_new_item_property  const& on_property
) {
    impl_load_definitions_<item_definition_handler>(
        "./data/items.dat", on_finish, on_property);
}

void load_entity_definitions(
    on_finish_entity_definition const& on_finish
  , on_add_new_entity_property  const& on_property
) {
    impl_load_definitions_<entity_definition_handler>(
        "./data/entities.dat", on_finish, on_property);
}

} //namespace boken
