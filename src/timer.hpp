#pragma once

#include "allocator.hpp"
#include "scope_guard.hpp"

#include <chrono>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstddef>

namespace boken {

class timer {
public:
    using clock_t    = std::chrono::high_resolution_clock;
    using time_point = clock_t::time_point;
    using duration   = clock_t::duration;
    using timer_data = uint64_t;

    //! a return value of 0 indicates the timer should be removed
    using callback_t = std::function<duration (duration, timer_data&)>;

    //! a cookie used to uniquely identify a timer
    struct key_t {
        size_t   index;
        uint32_t hash;

        constexpr bool operator==(key_t const k) const noexcept {
            return (k.index == index) && (k.hash == hash);
        }

        constexpr bool operator!=(key_t const k) const noexcept {
            return !(*this == k);
        }
    };

    key_t add(uint32_t const hash, duration const period, callback_t callback) {
        return add(hash, period, 0, std::move(callback));
    }

    //! @pre new timers cannot be added as a result of calling update().
    key_t add(
        uint32_t   const hash     //!< a 32-bit identifier (string hash)
      , duration   const period   //!< the period of the timer
      , timer_data const data     //!< timer specific, user-defined data
      , callback_t       callback //!< the timer action
    ) {
        BK_ASSERT(!updating_ && period.count() >= 0 && !!callback);

        auto const result = callbacks_.allocate(std::move(callback));
        auto const key    = key_t {result.second, hash};

        timers_.push_back({data, clock_t::now() + period, key});
        std::push_heap(begin(timers_), end(timers_), predicate_);

        return key;
    }

    //! @returns true if a timer matches key, false otherwise.
    //! @note timers can be removed as a result of calling update().
    bool remove(key_t const key) noexcept {
        auto const first = begin(timers_);
        auto const last  = end(timers_);
        auto const it    = std::find(first, last, key);

        if (it == last) {
            return false;
        }

        if (updating_) {
            it->deadline = time_point {};
            return true;
        }

        callbacks_.deallocate(it->key.index);

        if (it == first) {
            std::pop_heap(first, last, predicate_);
            timers_.pop_back();
        } else {
            timers_.erase(it);
            std::make_heap(begin(timers_), end(timers_), predicate_);
        }

        return true;
    }

    //! Trigger any ready timers.
    void update() {
        if (timers_.empty()) {
            return;
        }

        updating_ = true;
        auto on_exit = BK_SCOPE_EXIT {
            updating_ = false;
        };

        auto const now = clock_t::now();

        // remove "dead" timers that were marked as such by remove(). Timers
        // enter this state if they are removed during a callback.
        auto const remove_dead_timer = [&]() noexcept {
            auto const& top = timers_.front();
            if (top.deadline != time_point {}) {
                return false;
            }

            callbacks_.deallocate(top.key.index);
            std::pop_heap(begin(timers_), end(timers_), predicate_);
            timers_.pop_back();

            return true;
        };

        do {
            // check for any left over dead counters
            // we're done if it was the last one
            if (remove_dead_timer() && timers_.empty()) {
                break;
            }

            // the timer at the top of the heap is not ready yet, so no others
            // are ready either ~> done.
            auto const dt = now - timers_.front().deadline;
            if (dt.count() < 0) {
                break;
            }

            // make a copy of the timer key to do a sanity check.
            auto& t = timers_.front();
            auto const key = t.key;

            auto const period = callbacks_[t.key.index](dt, t.data);
            BK_ASSERT(period.count() >= 0
                   && !timers_.empty()
                   && timers_.front().key == key);

            // this timer was killed (removed) during the callback
            if (remove_dead_timer()) {
                continue;
            }

            // this timer is still alive

            auto const first = begin(timers_);
            auto const last  = end(timers_);

            // a period of 0 indicates the timer should not be repeated: so
            // don't push it back on the heap, and kill it's callback.
            if (period.count() <= 0) {
                callbacks_.deallocate(t.key.index);
                std::pop_heap(first, last, predicate_);
                timers_.pop_back();
                continue;
            }

            // repeat
            auto data = data_t {t.data, now + period, t.key};
            std::pop_heap(first, last, predicate_);
            timers_.back() = data;
            std::push_heap(first, last, predicate_);
        } while (!timers_.empty());
    }

private:
    struct data_t {
        timer_data data;
        time_point deadline;
        key_t      key;

        constexpr bool operator==(data_t const& other) const noexcept {
            return key == other.key;
        }

        constexpr bool operator==(key_t const& other) const noexcept {
            return key == other;
        }
    };

    static bool predicate_(data_t const& a, data_t const& b) noexcept {
        return a.deadline > b.deadline;
    }

    std::vector<data_t> timers_;
    contiguous_fixed_size_block_storage<callback_t> callbacks_;
    bool updating_ = false;
};

} //namespace boken
