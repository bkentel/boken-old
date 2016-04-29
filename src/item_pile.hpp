#pragma once

#include "types.hpp"

#include "bkassert/assert.hpp"

#include <vector>
#include <functional>
#include <algorithm>

namespace boken { class world; }
namespace boken { class game_database; }

namespace boken {

//! The container abstraction used to represent a collection of items.
//! For example, a pile of loot on the ground; the items inside a chest, or the
//! inventory of some entity.
//!
//! Item ownership is wholly managed by item_piles and the world. Namely, the
//! world briefly has ownership during item creation, but thereafter an
//! item_pile maintains ownership.
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

    //@{
    //! @note @p pred is called for all elements, and then again @p sink is
    //!       called for each element matched. i.e {p, p, p, p, s, s}.
    template <typename Predicate, typename Sink>
    void remove_if(int const* first, int const* last, Predicate pred, Sink sink) {
        BK_ASSERT((!!first && !!last) && ((first == last) || !!deleter_));

        if (first == last) {
            return;
        }

        int  index = 0;
        auto it    = first;

        for (auto& id : items_) {
            // done
            if (it == last) {
                break;
            }

            // no match; examine next
            if (*it != index) {
                ++index;
                continue;
            }

            if (!pred(id)) {
                ++index;
                continue;
            }

            ++it;
            ++index;

            sink(unique_item {id, *deleter_});
            id = item_instance_id {};
        }

        auto const first_i = items_.begin();
        auto const last_i  = items_.end();
        items_.erase(std::remove(first_i, last_i, item_instance_id {}), last_i);
    }

    template <typename Predicate, typename Sink>
    void remove_if(Predicate pred, Sink sink) {
        BK_ASSERT(empty() || !!deleter_);

        for (auto& id : items_) {
            if (!pred(id)) {
                continue;
            }

            sink(unique_item {id, *deleter_});
            id = item_instance_id {};
        }

        auto const first = items_.begin();
        auto const last  = items_.end();
        items_.erase(std::remove(first, last, item_instance_id {}), last);
    }

    //@}
private:
    item_deleter const* deleter_ {};
    std::vector<item_instance_id> items_;
};

inline auto begin(item_pile const& pile) noexcept { return pile.begin(); }
inline auto end(item_pile const& pile)   noexcept { return pile.end(); }

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
