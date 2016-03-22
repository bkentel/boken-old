#include "item.hpp"
#include "item_pile.hpp"
#include "bkassert/assert.hpp"
#include "forward_declarations.hpp"

namespace boken {

item_instance_id get_instance(item const& i) noexcept {
    return i.instance();
}

item_id get_id(item const& i) noexcept {
    return i.definition();
}

//=====--------------------------------------------------------------------=====
//                                  item
//=====--------------------------------------------------------------------=====
merge_item_result merge_item_piles(item_pile& from, item& to, item_merge_f const& f) {
    return merge_item_result::ok_merged_all; //TODO
}

//=====--------------------------------------------------------------------=====
//                                  item_pile
//=====--------------------------------------------------------------------=====
merge_item_result merge_item_piles(item_pile& from, item_pile& to, item_merge_f const& f) {
    using ir = item_merge_result;

    auto result = merge_item_result::ok_merged_none;
    auto const get_result = [&]() noexcept {
        return from.empty() ? merge_item_result::ok_merged_all
                            : result;
    };

    for (auto i = from.size(); i > 0; --i) {
        switch (f(from[i - 1])) {
        case ir::ok:
            to.add_item(from.remove_item(i - 1));
            result = merge_item_result::ok_merged_some;
            break;
        case ir::skip:
            continue;
        case ir::terminate:
            return get_result();
        default:
            BK_ASSERT(false);
            break;
        }
    }

    return get_result();
}

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
        return unique_item {item_instance_id {} , *deleter_};
    }

    items_.erase(it);
    return unique_item {id, *deleter_};
}

unique_item item_pile::remove_item(size_t const pos) {
    BK_ASSERT(pos < items_.size());
    BK_ASSERT(!!deleter_);

    auto const id = items_[pos];
    items_.erase(std::begin(items_) + static_cast<ptrdiff_t>(pos));
    return unique_item {id, *deleter_};
}

} //namespace boken
