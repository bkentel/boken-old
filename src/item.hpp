#pragma once

#include "types.hpp"
#include "item_def.hpp"
#include "object.hpp"

namespace boken { class game_database; }

namespace boken {

class item
  : public object<item_instance_id, item_definition> {
public:
    using object::object;
};

class item_pile {
public:
    size_t size()  const noexcept { return items_.size(); }
    auto   begin() const noexcept { return items_.begin(); }
    auto   end()   const noexcept { return items_.end(); }

    void add_item(item_instance_id item);

    template <typename InputIt>
    void add_items(InputIt first, InputIt last) {
        items_.insert(items_.end(), first, last);
    }

    void add_items(std::initializer_list<item_instance_id> items) {
        add_items(std::begin(items), std::end(items));
    }

    item_instance_id remove_item(item_instance_id id);
    item_instance_id remove_item(size_t pos);
private:
    std::vector<item_instance_id> items_;
};

} //namespace boken
