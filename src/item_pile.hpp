#pragma once

#include "types.hpp"
#include <vector>
#include <functional>

namespace boken { class world; }
namespace boken { class game_database; }

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

inline auto begin(item_pile const& pile) noexcept { return pile.begin(); }
inline auto end(item_pile const& pile)   noexcept { return pile.end(); }

//! Result of an item_merge_f
enum class item_merge_result : uint32_t {
    ok        //!< the item should be merged
  , skip      //!< the item should be skipped
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

void merge_into_pile(world& w, game_database const& db, unique_item&& itm, item_pile& pile);

} //namespace boken
