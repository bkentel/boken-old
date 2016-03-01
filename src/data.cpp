#include "data.hpp"
#include "entity_def.hpp"  // for entity_definition
#include "item_def.hpp"    // for item_definition
#include "tile.hpp"
#include "utility.hpp"     // for find_ptr_if
#include <vector>          // for vector
#include <unordered_map>

namespace {

template<
    typename AssociativeContainer
  , typename Key = typename AssociativeContainer::key_type>
auto find_or_nullptr(AssociativeContainer&& c, Key const& key) noexcept {
    using std::end;

    auto const it = c.find(key);
    return it == end(c) ? nullptr : std::addressof(it->second);
}

struct identity_hash {
    template <typename T, typename Tag>
    size_t operator()(boken::tagged_integral_value<T, Tag> const id) const noexcept {
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

    tile_map const& get_tile_map(tile_map_type const type) const noexcept final override {
        switch (type) {
        case tile_map_type::base:   break;
        case tile_map_type::entity: return tile_map_entities_;
        case tile_map_type::item:   return tile_map_items_;
        default:                    break;
        }

        return tile_map_base_;
    }
private:
    std::unordered_map<entity_id, entity_definition, identity_hash> entity_defs_;
    std::unordered_map<item_id,   item_definition,   identity_hash> item_defs_;

    tile_map tile_map_base_     {tile_map_type::base};
    tile_map tile_map_items_    {tile_map_type::item};
    tile_map tile_map_entities_ {tile_map_type::entity};
};

std::unique_ptr<game_database> make_game_database() {
    return std::make_unique<game_database_impl>();
}

game_database_impl::game_database_impl() {
    tile_map_entities_.texture_id = 1;

    using e_value_type = decltype(entity_defs_)::value_type;

    {
        auto       id      = std::string {"rat_small"};
        auto const id_hash = djb2_hash_32(id.data());

        auto edef = entity_definition {
            basic_definition {std::move(id), "small rat", "source", 0}
          , entity_id {id_hash}};

        edef.properties.add_or_update_properties({
            {entity_property::temperment, 0}
        });

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

    tile_map_entities_.tiles_x = sizeix {26};
    tile_map_entities_.tiles_y = sizeiy {17};
    tile_map_entities_.add_mapping(entity_id {djb2_hash_32("rat_small")}, 21 + 6 * 26);
    tile_map_entities_.add_mapping(entity_id {djb2_hash_32("player")}, 13 + 13 * 26);
}

} //namespace boken
