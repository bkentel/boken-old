#include "data.hpp"
#include "entity_def.hpp"  // for entity_definition
#include "item_def.hpp"    // for item_definition
#include "tile.hpp"
#include "serialize.hpp"
#include "forward_declarations.hpp"
#include "algorithm.hpp"

#include "bkassert/assert.hpp"

#include <unordered_map>
#include <cstdio>

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
        return find_(item_properties_, id);
    }

    string_view find(entity_property_id const id) const noexcept final override {
        return find_(entity_properties_, id);
    }

    tile_map const& get_tile_map(tile_map_type const type) const noexcept final override;
private:
    template <typename Id, typename Container>
    string_view find_(Container const& c, Id const id) const noexcept {
        auto const it = c.find(id);
        return it != end(c)
          ? string_view {it->second.name}
          : string_view {"{none such}"};
    }

    void load_entity_defs_();
    void load_item_defs_();

    std::unordered_map<entity_id, entity_definition, identity_hash> entity_defs_;
    std::unordered_map<item_id,   item_definition,   identity_hash> item_defs_;

    struct property_data {
        serialize_data_type type;
        std::string         name;
        int32_t             count;
    };

    std::unordered_map<entity_property_id, property_data, identity_hash> entity_properties_;
    std::unordered_map<item_property_id,   property_data, identity_hash> item_properties_;

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

namespace {

template <typename Container>
auto load_definition_(Container& c, tile_map& tmap) {
    using def_t = typename std::decay_t<Container>::mapped_type;

    return [&](def_t const& def) {
        auto const tile_index =
            def.properties.value_or(djb2_hash_32c("tile_index"), 0);

        auto const result = c.insert({def.id, std::move(def)});
        if (!result.second) {
            BK_ASSERT(false); //TODO collision
        }

        tmap.add_mapping(def.id, tile_index);
    };
}

template <typename Container>
auto load_property_(Container& c) {
    return [&](string_view         const string
             , uint32_t            const hash
             , serialize_data_type const type
             , uint32_t            const //value //TODO
        ) {
            using id_t = typename std::decay_t<Container>::key_type;

            auto const id = id_t {hash};
            auto const it = c.find(id);

            if (it == end(c)) {
                c.insert({id, {type, string.to_string(), 1}});
                return true;
            }

            if (it->second.name != string) {
                BK_ASSERT(false); //TODO collision
            }

            if (it->second.type != type) {
                //TODO type differs between property usages
                printf("warning type differs for property \"%s\"\n"
                     , string.data());
            }

            ++it->second.count;

            return true;
        };
}

} // namespace

void game_database_impl::load_entity_defs_() {
    load_entity_definitions(load_definition_(entity_defs_, tile_map_entities_)
                          , load_property_(entity_properties_));
}

void game_database_impl::load_item_defs_() {
    load_item_definitions(load_definition_(item_defs_, tile_map_items_)
                        , load_property_(item_properties_));
}

game_database_impl::game_database_impl() {
    load_entity_defs_();
    load_item_defs_();
}

item_definition const* find(game_database const& db, item_id const id) noexcept {
    return db.find(id);
}

entity_definition const* find(game_database const& db, entity_id const id) noexcept {
    return db.find(id);
}

} //namespace boken
