#pragma once

#include "types.hpp"
#include <vector>

namespace boken {

class item_pile {
public:
    ~item_pile();
    item_pile();
    item_pile(item_pile&&) = default;
    item_pile& operator=(item_pile&&) = default;

    item_pile(item_pile const&) = delete;
    item_pile& operator=(item_pile const&) = delete;

    bool   empty() const noexcept { return items_.empty(); }
    size_t size()  const noexcept { return items_.size(); }
    auto   begin() const noexcept { return items_.begin(); }
    auto   end()   const noexcept { return items_.end(); }

    item_instance_id operator[](size_t index) const noexcept;

    void add_item(unique_item item);

    //! return an empty unique_item if no item with id exists
    unique_item remove_item(item_instance_id id);
    unique_item remove_item(size_t pos);
private:
    item_deleter const* deleter_ {};
    std::vector<item_instance_id> items_;
};

} //namespace boken
