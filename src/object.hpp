#pragma once

#include "item_pile.hpp"
#include "forward_declarations.hpp"

#include <type_traits>
#include <utility>
#include <initializer_list>

#include <cstdint>

namespace boken { class game_database; }

namespace boken {

template <typename Derived, typename Definition, typename Instance>
class object {
public:
    using instance_id_t    = Instance;
    using definition_t     = Definition;
    using definition_id_t  = typename Definition::definition_id_t;
    using properties_t     = typename Definition::properties_t;
    using property_t       = typename Definition::property_t;
    using property_value_t = typename Definition::property_value_t;
    using property_pair_t  = std::pair<property_t, property_value_t>;

    object(instance_id_t const instance, definition_id_t const id)
      : instance_id_ {instance}
      , id_ {id}
    {
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Ids
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    instance_id_t   instance()   const noexcept { return instance_id_; }
    definition_id_t definition() const noexcept { return id_; }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Items
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //! Take ownership of @p itm.
    //! @note No checking is performed as to whether @p itm can be held by this
    //!       object or not. A check must first be performed with can_add_item.
    void add_item(unique_item&& itm) {
        items_.add_item(std::move(itm));
    }

    item_pile const& items() const noexcept { return items_; }
    item_pile&       items()       noexcept { return items_; } // TODO

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //                              Properties
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    property_value_t property_value_or(
        definition_t     const& def
      , property_t       const  property
      , property_value_t const  fallback
    ) const noexcept {
        BK_ASSERT(def.id == definition());
        return get_property_value_or(
            property, fallback, properties_, def.properties);
    }

    property_value_t property_value_or(
        game_database    const& db
      , property_t       const  property
      , property_value_t const  fallback
    ) const noexcept {
        auto const def = find(db, definition());
        return def
          ? property_value_or(*def, property, fallback)
          : get_property_value_or(property, fallback, properties_);
    }

    bool add_or_update_property(
        property_t       const property
      , property_value_t const value
    ) {
        return properties_.add_or_update_property(property, value);
    }

    bool add_or_update_property(property_pair_t const property_pair) {
        return add_or_update_property(property_pair.first, property_pair.second);
    }

    template <typename InputIt>
    int add_or_update_properties(InputIt first, InputIt last) {
        return properties_.add_or_update_properties(first, last);
    }

    int add_or_update_properties(std::initializer_list<property_pair_t> properties) {
        return properties_.add_or_update_properties(properties);
    }

    bool remove_property(property_t const property) {
        return properties_.remove_property(property);
    }
private:
    instance_id_t   instance_id_ {0};
    definition_id_t id_          {0};
    properties_t    properties_;
    item_pile       items_;
};

} //namespace boken
