#pragma once

#include "bkassert/assert.hpp"

#include <utility>
#include <vector>
#include <memory>

#include <cstddef>
#include <cstdint>

namespace boken {

//! fixed size block allocator
//! @note this does not conform to the stl allocator interface
template <typename T>
class contiguous_fixed_size_block_storage {
    struct block_data_t {
        uint32_t next;
        uint32_t flags;
    };

    struct construct_t {};

    union block_t {
        template <typename... Args>
        block_t(construct_t, Args&&... args)
          : data {std::forward<Args>(args)...}
        {
        }

        block_t() noexcept
          : info {}
        {
        }

        block_t(block_t&& other) noexcept
          : data {std::move(other.data)}
        {
        }

        ~block_t() {}

        block_data_t info;
        T            data;
    };
public:
    size_t next_block_id() const noexcept {
        return next_free_ + 1; // ids start at 1
    }

    template <typename... Args>
    std::pair<T*, size_t> allocate(Args&&... args) {
        // TODO: what if the constructor throws here?

        // do we need to allocate a new block of storage?
        if (next_free_ >= data_.size()) {
            data_.emplace_back(construct_t {}, std::forward<Args>(args)...);
            return {std::addressof(data_.back().data), ++next_free_}; // ids start at 1
        }

        // no, we have a free block somewhere to use
        block_t&   block = data_[next_free_];
        auto const i     = block.info.next;

        // reset the active union member
        block.info.~block_data_t();

        auto const p = std::addressof(block.data);

        new (p) T {std::forward<Args>(args)...};

        auto result = std::make_pair(p, next_free_ + 1); // ids start at 1
        next_free_ = i;

        return result;
    }

    //! free the block with the given id by calling its destructor
    void deallocate(size_t const i) noexcept {
        BK_ASSERT(i >= 1 && i <= data_.size()); // ids start at 1

        auto const index = static_cast<uint32_t>(i) - 1;
        data_[index].data.~T();
        new (&data_[index].info) block_data_t {next_free_, 0x00DEAD00u};

        next_free_ = index;
    }

    size_t capacity() const noexcept { return data_.size(); }

    T&       operator[](size_t const i)       noexcept { return data_[i - 1].data; }
    T const& operator[](size_t const i) const noexcept { return data_[i - 1].data; }
private:
    std::vector<block_t> data_;
    uint32_t             next_free_ {};
};

} //namespace boken
