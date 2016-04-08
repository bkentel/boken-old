#include "data.hpp"
#include "entity_def.hpp"  // for entity_definition
#include "item_def.hpp"    // for item_definition
#include "tile.hpp"
#include "utility.hpp"     // for find_ptr_if
#include "serialize.hpp"

#include <vector>          // for vector
#include <unordered_map>

namespace {

template<
    typename AssociativeContainer
  , typename Key = typename AssociativeContainer::key_type>
auto find_or_nullptr(AssociativeContainer&& c, Key const& key) noexcept {
    using std::end;

    auto const it = c.find(key);
    return it == end(c)
      ? nullptr
      : std::addressof(it->second);
}

struct identity_hash {
    template <typename T, typename Tag>
    size_t operator()(boken::tagged_value<T, Tag> const id) const noexcept {
        return boken::value_cast(id);
    }
};

} //namespace anonymous

namespace boken {

game_database::~game_database() = default;

class game_database_impl final : public game_database {
public:
    game_database_impl();

    item_definition const* find(item_id const id) const noexcept final override {
        return find_or_nullptr(item_defs_, id);
    }

    entity_definition const* find(entity_id const id) const noexcept final override {
        return find_or_nullptr(entity_defs_, id);
    }

    string_view find(item_property_id const id) const noexcept final override {
        auto const it = item_properties_.find(id);
        return it != end(item_properties_)
          ? string_view {it->second}
          : string_view {"{none such}"};
    }

    string_view find(entity_property_id const id) const noexcept final override {
        auto const it = entity_properties_.find(id);
        return it != end(entity_properties_)
          ? string_view {it->second}
          : string_view {"{none such}"};
    }

    tile_map const& get_tile_map(tile_map_type const type) const noexcept final override;
private:
    void load_entity_defs_();
    void load_item_defs_();

    std::unordered_map<entity_id, entity_definition, identity_hash> entity_defs_;
    std::unordered_map<item_id,   item_definition,   identity_hash> item_defs_;

    std::unordered_map<entity_property_id, std::string, identity_hash> entity_properties_;
    std::unordered_map<item_property_id,   std::string, identity_hash> item_properties_;

    tile_map tile_map_base_     {tile_map_type::base,   0, sizei32x {18}, sizei32y {18}, sizei32x {16}, sizei32y {16}};
    tile_map tile_map_entities_ {tile_map_type::entity, 1, sizei32x {18}, sizei32y {18}, sizei32x {26}, sizei32y {17}};
    tile_map tile_map_items_    {tile_map_type::item,   2, sizei32x {18}, sizei32y {18}, sizei32x {16}, sizei32y {16}};
};

std::unique_ptr<game_database> make_game_database() {
    return std::make_unique<game_database_impl>();
}

tile_map const&
game_database_impl::get_tile_map(tile_map_type const type) const noexcept {
    switch (type) {
    case tile_map_type::base:   break;
    case tile_map_type::entity: return tile_map_entities_;
    case tile_map_type::item:   return tile_map_items_;
    default:                    break;
    }

    return tile_map_base_;
}

void game_database_impl::load_entity_defs_() {
    {
        auto       id      = std::string {"rat_small"};
        auto const id_hash = djb2_hash_32(id.data());

        auto edef = entity_definition {
            basic_definition {std::move(id), "small rat", "source", 0}
          , entity_id {id_hash}};

        entity_defs_.insert(std::make_pair(entity_id {id_hash}, std::move(edef)));
    }

    {
        auto       id      = std::string {"player"};
        auto const id_hash = djb2_hash_32(id.data());

        entity_defs_.insert(std::make_pair(
            entity_id {id_hash}
          , entity_definition {
                basic_definition {std::move(id), "player", "source", 0}
              , entity_id {id_hash}}));
    }

    tile_map_entities_.add_mapping(entity_id {djb2_hash_32("rat_small")}, 21 + 6 * 26);
    tile_map_entities_.add_mapping(entity_id {djb2_hash_32("player")}, 13 + 13 * 26);
}

void game_database_impl::load_item_defs_() {
    load_item_definitions(
        [&](item_definition const& def) {
            auto const id = def.id;
            auto const tile_index = def.properties.value_or(djb2_hash_32c("tile_index"), 0);

            auto const result = item_defs_.insert({id, std::move(def)});
            if (!result.second) {
                BK_ASSERT(false); //TODO collision
            }

            tile_map_items_.add_mapping(id, tile_index);
        }
      , [&](string_view const string, uint32_t const hash, serialize_data_type const type, uint32_t const value) {
            auto const id = item_property_id {hash};
            auto const it = item_properties_.find(id);
            if (it == end(item_properties_)) {
                item_properties_.insert({id, string.to_string()});
            } else if (string != it->second) {
                BK_ASSERT(false); //TODO collision
            }

            return true;
        });
}

game_database_impl::game_database_impl() {
    load_entity_defs_();
    load_item_defs_();
}

bool has_property(game_database const& data, entity_id const& def, entity_property_id const property) noexcept {
    auto const edef = data.find(def);
    return !!edef && has_property(*edef, property);
}

bool has_property(entity_definition const& def, entity_property_id const property) noexcept {
    return def.properties.has_property(property);
}

entity_property_value property_value_or(game_database const& data, entity_id const& def, entity_property_id const property, entity_property_value const value) noexcept {
    auto const edef = data.find(def);
    return !edef
      ? value
      : property_value_or(*edef, property, value);
}

entity_property_value property_value_or(entity_definition const& def, entity_property_id const property, entity_property_value const value) noexcept {
    return def.properties.value_or(property, value);
}

bool has_property(game_database const& data, item_id const& def, item_property_id const property) noexcept {
    auto const idef = data.find(def);
    return !!idef && has_property(*idef, property);
}

bool has_property(item_definition const& def, item_property_id const property) noexcept {
    return def.properties.has_property(property);
}

item_property_value property_value_or(game_database const& data, item_id const& def, item_property_id const property, item_property_value const value) noexcept {
    auto const idef = data.find(def);
    return !idef
      ? value
      : property_value_or(*idef, property, value);
}

item_property_value property_value_or(item_definition const& def, item_property_id const property, item_property_value const value) noexcept {
    return def.properties.value_or(property, value);
}

item_definition const* find(game_database const& db, item_id const id) noexcept {
    return db.find(id);
}

entity_definition const* find(game_database const& db, entity_id const id) noexcept {
    return db.find(id);
}

} //namespace boken
