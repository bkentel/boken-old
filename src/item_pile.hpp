#pragma once

#include "types.hpp"
#include "scope_guard.hpp"
#include "context.hpp"

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
    //! Remove items from the pile at the indicies in the range [first, last).
    //! @param pred A function of the form (unique_item&& i) -> any.
    //!             If the unique_item has ownership taken from it, it indicates
    //!             the predicate is true, false otherwise.
    template <typename FwdIt, typename Predicate>
    int remove_if(FwdIt const first, FwdIt const last, Predicate pred) {
        BK_ASSERT(!!deleter_);

        if (first == last) {
            return 0;
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

            remove_id_(id, pred);

            ++it;
            ++index;
        }

        return remove_dead_items_();
    }

    template <typename Predicate>
    int remove_if(Predicate pred) {
        BK_ASSERT(!!deleter_);

        for (auto& id : items_) {
            remove_id_(id, pred);
        }

        return remove_dead_items_();
    }

    template <typename FwdIt, typename Transform, typename Predicate>
    int remove_if2(FwdIt const first, FwdIt const last, Transform trans, Predicate pred) {
        // indicies pile
        // [ ]      [4]
        // [ ]      [1]
        // [6]      [6]
        // [ ]      [7]

        using value_type = std::decay_t<decltype(trans(*first))>;
        static_assert(std::is_same<item_instance_id, value_type>::value, "");

        auto       p_it   = items_.begin();
        auto const p_last = items_.end();

        for (auto it = first; it != last; ++it, ++p_it) {
            for (auto const id = trans(*it); *p_it != id; ++p_it) {
                BK_ASSERT(p_it != p_last);
            }

            remove_id_(*p_it, pred);
        }

        return remove_dead_items_();
    }

    //@}
private:
    template <typename Predicate>
    void remove_id_(item_instance_id& id, Predicate&& pred) {
        // make an owning pointer to the item
        auto itm = unique_item {id, *deleter_};

        // if the predicate throws, an item could end up lost; ensure that
        // cleanup always happens properly.
        auto on_exit = BK_SCOPE_EXIT {
            if (itm) {
                // the predicate didn't take ownership
                itm.release();
            } else {
                // the predicate did take ownership -- zero out the id
                id = item_instance_id {};
            }
        };

        pred(std::move(itm));
    }

    int remove_dead_items_() {
        // remove any of the items that has ownership taken and were zeroed out
        auto const id    = item_instance_id {};
        auto const first = items_.begin();
        auto const last  = items_.end();

        auto const size_before = static_cast<int>(items_.size());
        items_.erase(std::remove(first, last, id), last);
        auto const size_after = static_cast<int>(items_.size());

        return size_before - size_after;
    }

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

void merge_into_pile(context ctx, unique_item itm_ptr, item_descriptor itm
    , item_pile& dst);

} //namespace boken
