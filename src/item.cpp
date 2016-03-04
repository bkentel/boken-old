#include "item.hpp"
#include "item_pile.hpp"
#include "bkassert/assert.hpp"

namespace boken {

//=====--------------------------------------------------------------------=====
//                                  item_pile
//=====--------------------------------------------------------------------=====
item_pile::~item_pile() {
    BK_ASSERT(items_.empty() || !!deleter_);

    for (auto const& id : items_) {
        unique_item {id, *deleter_};
    }
}

item_pile::item_pile()
{
}

item_instance_id item_pile::operator[](size_t const index) const noexcept {
    BK_ASSERT(index < items_.size());
    return items_[index];
}

void item_pile::add_item(unique_item item) {
    if (!deleter_) {
        deleter_ = &item.get_deleter();
    }

    items_.push_back(item.release());
}

unique_item item_pile::remove_item(item_instance_id const id) {
    BK_ASSERT(!!deleter_);

    auto const it = std::find(std::begin(items_), std::end(items_), id);
    if (it == std::end(items_)) {
        return unique_item {item_instance_id {0} , *deleter_};
    }

    items_.erase(it);
    return unique_item {id, *deleter_};
}

unique_item item_pile::remove_item(size_t const pos) {
    BK_ASSERT(pos < items_.size());
    BK_ASSERT(!!deleter_);

    auto const id = items_[pos];
    items_.erase(std::begin(items_) + pos);
    return unique_item {id, *deleter_};
}

} //namespace boken
