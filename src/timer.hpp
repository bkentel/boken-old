#pragma once

#include "allocator.hpp"

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

    using callback_t = std::function<duration (duration, timer_data&)>;

    size_t add(duration const period, callback_t callback) {
        return add(period, 0, std::move(callback));
    }

    size_t add(duration const period, timer_data const data, callback_t callback) {
        auto const result = callbacks_.allocate(std::move(callback));
        push_({clock_t::now() + period, result.second, data});
        return result.second;
    }

    void update() {
        if (timers_.empty()) {
            return;
        }

        auto const now = clock_t::now();

        while (!timers_.empty()) {
            auto const t  = timers_.front();
            auto const dt = now - t.deadline;
            if (dt.count() < 0) {
                break;
            }

            pop_();

            auto       data   = t.data;
            auto const period = callbacks_[t.index](dt, data);
            if (period.count() <= 0) {
                callbacks_.deallocate(t.index);
                continue;
            }

            push_({now + period, t.index, data});
        }
    }
private:
    struct data_t {
        time_point deadline;
        size_t     index;
        timer_data data;
    };

    void push_(data_t const data) {
        timers_.push_back(data);
        std::push_heap(begin(timers_), end(timers_)
            , [](data_t const& a, data_t const& b) noexcept {
                return a.deadline > b.deadline;
            });
    }

    void pop_() noexcept {
        std::pop_heap(begin(timers_), end(timers_)
            , [](data_t const& a, data_t const& b) noexcept {
                return a.deadline > b.deadline;
            });
        timers_.pop_back();
    }

    std::vector<data_t> timers_;
    contiguous_fixed_size_block_storage<callback_t> callbacks_;
};

} //namespace boken
