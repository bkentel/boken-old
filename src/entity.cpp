#include "entity.hpp"
#include "data.hpp"

namespace boken {

bool entity::has_property(
    game_database const& data
  , entity_property const p
) const noexcept {
    if (properties_.has_property(p)) {
        return true;
    }

    auto const def = data.find(definition());
    return def && def->properties.has_property(p);
}

entity::property_value entity::property_value_or(
    game_database const& data
  , entity_property const p
  , property_value const value
) const noexcept {
    if (properties_.has_property(p)) {
        return properties_.value_or(p, 0);
    }

    auto const def = data.find(definition());
    return def && def->properties.value_or(p, value);
}

bool entity::add_or_update_property(
    entity_property const p
  , property_value const value
) {
    return properties_.add_or_update_property(p, value);
}

int entity::add_or_update_properties(std::initializer_list<property_pair> properties) {
    return properties_.add_or_update_properties(properties);
}

bool entity::remove_property(entity_property const p) {
    return properties_.remove_property(p);
}

} //namespace boken
