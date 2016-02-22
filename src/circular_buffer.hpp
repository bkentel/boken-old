#pragma once

#include <bkassert/assert.hpp>

#include <iterator>
#include <vector>
#include <cstddef>

namespace boken {

template <typename T>
class simple_circular_buffer_iterator : public std::iterator_traits<T*> {
    using this_t = simple_circular_buffer_iterator<T>;
public:
    using reference = typename std::iterator_traits<T*>::reference;
    using pointer   = typename std::iterator_traits<T*>::pointer;

    simple_circular_buffer_iterator(T* const front_p, size_t const front, size_t const capacity)
      : p_        {front_p}
      , delta_    {0}
      , front_    {front}
      , capacity_ {capacity}
    {
    }

    reference operator*() const noexcept {
        return *(p_ + (front_ % capacity_));
    }

    pointer operator->() const noexcept {
        return &**this;
    }

    void operator++() noexcept {
        front_ = (front_ + 1) % capacity_;
        ++delta_;
    }

    simple_circular_buffer_iterator operator++(int) noexcept {
        auto result = *this;
        ++(*this);
        return result;
    }

    bool operator<(this_t const& other) const noexcept {
        return (!p_ && !other.p_) ? false
             : (!p_)              ? false
             : (!other.p_)        ? true
                                  : p_ < other.p_;
    }

    bool operator==(this_t const& other) const noexcept {
        return (!p_ && !other.p_) ? true
             : (!p_)              ? compare_end_(other, *this)
             : (!other.p_)        ? compare_end_(*this, other)
                                  : compare_(*this, other);
    }

    bool operator!=(this_t const& other) const noexcept {
        return !(*this == other);
    }
private:
    bool compare_(this_t const& it0, this_t const& it1) const noexcept {
        return (it0.p_        == it1.p_)
            && (it0.delta_    == it1.delta_)
            && (it0.front_    == it1.front_)
            && (it0.capacity_ == it1.capacity_);
    }

    bool compare_end_(this_t const& it, this_t const& end) const noexcept {
        return it.delta_ && (it.front_    == end.front_)
                         && (it.capacity_ == end.capacity_);
    }

    T*        p_        {};
    ptrdiff_t delta_    {};
    size_t    front_    {};
    size_t    capacity_ {};
};

template <typename T>
class simple_circular_buffer {
public:
    using iterator       = simple_circular_buffer_iterator<T>;
    using const_iterator = simple_circular_buffer_iterator<std::add_const_t<T>>;

    explicit simple_circular_buffer(size_t const capacity)
      : capacity_ {capacity}
      , front_    {0}
    {
    }

    template <typename U>
    void push(U&& data) {
        static_assert(std::is_same<T, std::decay_t<U>>::value, "");

        if (data_.size() < capacity_) {
            data_.push_back(std::forward<U>(data));
            return;
        }

        data_[(front_ + data_.size()) % capacity_].~T();
        new (&data_[(front_ + data_.size()) % capacity_]) T {std::forward<U>(data)};

        front_ = (data_.size() < capacity_)
          ? 0
          : (front_ + 1) % capacity_;
    }

    T const& operator[](ptrdiff_t const where) const {
        BK_ASSERT(std::abs(where) < data_.size());

        auto const i = (where >= 0
          ? (front_ + where)
          : (front_ + data_.size() - where)) % data_.size();

        return data_[i];
    }

    size_t size() const noexcept {
        return data_.size();
    }

    const_iterator begin() const noexcept {
        return {data_.data(), front_, data_.size()};
    }

    const_iterator end() const noexcept {
        return {nullptr, front_, data_.size()};
    }

    iterator begin() noexcept {
        return {data_.data(), front_, data_.size()};
    }

    iterator end() noexcept {
        return {nullptr, front_, data_.size()};
    }
private:
    size_t         capacity_;
    size_t         front_;
    std::vector<T> data_;
};

} //namespace boken
