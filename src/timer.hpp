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

    key_t add(uint32_t const hash, duration const period, timer_data const data, callback_t callback) {
        auto const result = callbacks_.allocate(std::move(callback));
        auto const key    = key_t {result.second, hash};

        push_({data, clock_t::now() + period, key});

        return key;
    }

    void remove(key_t const key) noexcept {
        auto const first = begin(timers_);
        auto const last  = end(timers_);
        auto const it    = std::find(first, last, key);

        BK_ASSERT(it != last);

        callbacks_.deallocate(it->key.index);

        if (it == first) {
            pop_();
        } else if (it == first + static_cast<ptrdiff_t>(timers_.size() - 1u)) {
            timers_.pop_back();
        } else {
            timers_.erase(it);
            make_();
        }
    }

    void update() {
        if (timers_.empty()) {
            return;
        }

        auto const now = clock_t::now();

        while (!timers_.empty()) {
            // the closest timer is not ready yet, so no other are -> done
            auto const dt = now - timers_.front().deadline;
            if (dt.count() < 0) {
                break;
            }

            // make a copy of the closest timer record as it could get changed
            // after the handler is called.
            auto t = timers_.front();

            // first, complete the callback without updating the heap. This is
            // important because the callback itself could remove another timer
            // or even this timer itself.
            auto const period = callbacks_[t.key.index](dt, t.data);

            // inspect the top of the heap again to ensure this timer is still
            // alive and unchanged.
            if (timers_.empty() || timers_.front().key != t.key) {
                // the heap has changed, so we need to check if this timer still
                // exists

                auto const first = begin(timers_);
                auto const last  = end(timers_);
                auto const it    = std::find(first, last, t.key);
                if (it == last) {
                    continue; // the timer has been removed
                } else if (it != first) {
                    BK_ASSERT(false); // TODO: not sure this is possible
                }
            }

            pop_();
            if (period.count() > 0) {
                push_({t.data, now + period, t.key});
            } else {
                callbacks_.deallocate(t.key.index);
            }
        }
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

    void push_(data_t const data) {
        timers_.push_back(data);
        std::push_heap(begin(timers_), end(timers_), predicate_);
    }

    void pop_() noexcept {
        std::pop_heap(begin(timers_), end(timers_), predicate_);
        timers_.pop_back();
    }

    void make_() noexcept {
        std::make_heap(begin(timers_), end(timers_), predicate_);
    }

    std::vector<data_t> timers_;
    contiguous_fixed_size_block_storage<callback_t> callbacks_;
};

} //namespace boken
