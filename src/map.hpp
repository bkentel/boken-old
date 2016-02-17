//#pragma once
//
//#include "types.hpp"
//
//#include <vector>
//#include <type_traits>
//
//namespace boken {
//
//template <typename T>
//class map_data {
//public:
//    using value_type = T;
//    using size_type  = boken::size_type<int>;
//
//    static_assert(std::is_standard_layout<value_type>::value, "");
//
//    map_data(size_type const size_x, size_type const size_y, T const value = T{})
//      : size_x_ {size_x}
//      , size_y_ {size_y}
//      , data_(static_cast<size_t>(area()), value)
//    {
//    }
//
//    size_type width()  const noexcept { return size_x_; }
//    size_type height() const noexcept { return size_y_; }
//
//    size_type::type area() const noexcept {
//        return value_cast(width()) * value_cast(height());
//    }
//
//    T get_value(ptrdiff_t const x, ptrdiff_t const y) const noexcept {
//        return data_[value_cast(x) + value_cast(y) * value_cast(size_y_)];
//    }
//
//    void set_value(ptrdiff_t const x, ptrdiff_t const y, T const n) noexcept {
//        data_[value_cast(x) + value_cast(y) * value_cast(size_y_)] = n;
//    }
//private:
//    size_type size_x_;
//    size_type size_y_;
//    std::vector<T> data_;
//};
//
//} //namespace boken
