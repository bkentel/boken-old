#include "entity.hpp"

namespace boken {

bool entity::can_add_item(item const& itm) {
    return true;
}

void entity::add_item(unique_item itm) {
    items_.add_item(std::move(itm));
}

} //namespace boken
