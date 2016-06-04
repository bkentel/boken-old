#pragma once

#include "bkassert/assert.hpp"

#include <iterator>
#include <vector>
#include <cstddef>

namespace boken {

//! Random access iterator for circular_buffer
template <typename T>
class simple_circular_buffer_iterator : public std::iterator_traits<T*> {
    using this_t = simple_circular_buffer_iterator<T>;
    friend simple_circular_buffer_iterator<T const>;
public:
    using reference       = typename std::iterator_traits<T*>::reference;
    using pointer         = typename std::iterator_traits<T*>::pointer;
    using difference_type = typename std::iterator_traits<T*>::difference_type;

    simple_circular_buffer_iterator(T* const front_p, size_t const front, size_t const capacity)
      : p_        {front_p}
      , delta_    {0}
      , front_    {front}
      , capacity_ {capacity}
    {
    }

    template <typename U, typename = std::enable_if_t<!std::is_const<U>::value>>
    simple_circular_buffer_iterator(simple_circular_buffer_iterator<U> const& other)
      : p_        {other.p_}
      , delta_    {other.delta_}
      , front_    {other.front_}
      , capacity_ {other.capacity_}
    {
        static_assert(std::is_same<std::decay_t<U>, std::decay_t<T>>::value, "");
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

    template <typename U>
    difference_type operator-(simple_circular_buffer_iterator<U> const rhs) noexcept {
        static_assert(std::is_same<std::decay_t<U>, std::decay_t<T>>::value, "");

        auto const diff = [](auto const a, auto const b) noexcept {
            return static_cast<difference_type>(
                static_cast<difference_type>(a)
              - static_cast<difference_type>(b));
        };

        return ( p_ &&  rhs.p_) ? diff(front_, rhs.front_)
             : (!p_ &&  rhs.p_) ? diff(rhs.capacity_, rhs.delta_)
             : ( p_ && !rhs.p_) ? diff(capacity_, delta_)
             : difference_type {};
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

//! A fixed size (at run time) circular buffer.
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
        static_assert(std::is_constructible<T, std::add_rvalue_reference_t<U>>::value, "");

        if (data_.size() < capacity_) {
            data_.push_back(std::forward<U>(data));
            return;
        }

        auto const i = (front_ + data_.size()) % capacity_;

        data_[i].~T();
        new (&data_[i]) T {std::forward<U>(data)};

        front_ = (data_.size() < capacity_)
          ? 0
          : (front_ + 1) % capacity_;
    }

    T const& operator[](ptrdiff_t const where) const {
        BK_ASSERT(static_cast<size_t>(std::abs(where)) < data_.size());

        auto const size = data_.size();

        auto const i = static_cast<size_t>(
            static_cast<ptrdiff_t>(front_)
          + static_cast<ptrdiff_t>(size) + where) % size;

        return data_[i];
    }

    size_t size() const noexcept {
        return data_.size();
    }

    const_iterator begin() const noexcept {
        return const_cast<simple_circular_buffer*>(this)->begin();
    }

    const_iterator end() const noexcept {
        return const_cast<simple_circular_buffer*>(this)->end();
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

template <typename T>
auto begin(simple_circular_buffer<T> const& buffer) noexcept {
    return buffer.begin();
}

template <typename T>
auto end(simple_circular_buffer<T> const& buffer) noexcept {
    return buffer.end();
}

} //namespace boken
