#pragma once

#include "types.hpp"
#include "forward_declarations.hpp"
#include <vector>
#include <functional>

namespace boken { class entity; }
namespace boken { class item; }

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

//! check(item_instance_id) = 0 ~> ok
//! check(item_instance_id) = 1 ~> skip
//! check(item_instance_id) = 2 ~> terminate
//! check(item_instance_id) = X ~> terminate
template <typename Check>
int merge_item_piles(item_pile& from, item_pile& to, Check check) {
    int n {};
    for (auto i = from.size(); i > 0; --i) {
        switch (check(from[i - 1])) {
        case 0:  to.add_item(from.remove_item(i - 1)); ++n; break;
        case 1:  continue;
        case 2:  return n;
        default: return n;
        }
    }

    return n;
}

//! Result of an item_merge_f
enum class item_merge_result : uint32_t {
    ok        //!< the item should be merged
  , skip      //!< the item shoudl be skipped
  , terminate //!< all subsequent merges should be skipped
};

//! result of merging two item_piles
enum class merge_item_result : uint32_t {
    ok_merged_none //!< ok, but nothing was moved
  , ok_merged_some //!< ok, at least one item was moved, but not all
  , ok_merged_all  //!< ok, all items were moved and the source pile is not empty
  , failed_bad_source
  , failed_bad_destination
};

using item_merge_f = std::function<item_merge_result (item_instance_id)>;

merge_item_result merge_item_piles(item_pile& from, item& to, item_merge_f const& f);
merge_item_result merge_item_piles(item_pile& from, entity& to, item_merge_f const& f);
merge_item_result merge_item_piles(item_pile& from, item_pile& to, item_merge_f const& f);

} //namespace boken
