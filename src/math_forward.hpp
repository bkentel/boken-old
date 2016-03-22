#pragma once

//
// Forward declarations for mathematical types, and aliases thereof.
//

#include <cstdint>

namespace boken {

//=====--------------------------------------------------------------------=====
//                                Tags
//=====--------------------------------------------------------------------=====
struct tag_axis_x    {};
struct tag_axis_y    {};
struct tag_axis_none {};
struct tag_point     {};
struct tag_vector    {};

//=====--------------------------------------------------------------------=====
//                                Types
//=====--------------------------------------------------------------------=====
template <typename T, typename TagAxis, typename TagType>
class basic_1_tuple;

template <typename T, typename TagType>
class basic_2_tuple;

template <typename T>
class axis_aligned_rect;

//=====--------------------------------------------------------------------=====
//                                Type Aliases
//=====--------------------------------------------------------------------=====
template <typename T>
using size_type = basic_1_tuple<T, tag_axis_none, tag_vector>;

template <typename T>
using size_type_x = basic_1_tuple<T, tag_axis_x, tag_vector>;

template <typename T>
using size_type_y = basic_1_tuple<T, tag_axis_y, tag_vector>;

template <typename T>
using offset_type = basic_1_tuple<T, tag_axis_none, tag_point>;

template <typename T>
using offset_type_x = basic_1_tuple<T, tag_axis_x, tag_point>;

template <typename T>
using offset_type_y = basic_1_tuple<T, tag_axis_y, tag_point>;

template <typename T>
using point2 = basic_2_tuple<T, tag_point>;

template <typename T>
using vec2 = basic_2_tuple<T, tag_vector>;

using point2f = point2<float>;
using vec2f   = vec2<float>;
using sizef   = size_type<float>;
using sizefx  = size_type_x<float>;
using sizefy  = size_type_y<float>;
using offf    = offset_type<float>;
using offfx   = offset_type_x<float>;
using offfy   = offset_type_y<float>;

using point2i32 = point2<int32_t>;
using vec2i32   = vec2<int32_t>;
using sizei32   = size_type<int32_t>;
using sizei32x  = size_type_x<int32_t>;
using sizei32y  = size_type_y<int32_t>;
using offi32    = offset_type<int32_t>;
using offi32x   = offset_type_x<int32_t>;
using offi32y   = offset_type_y<int32_t>;

using point2i16 = point2<int16_t>;
using vec2i16   = vec2<int16_t>;
using sizei16   = size_type<int16_t>;
using sizei16x  = size_type_x<int16_t>;
using sizei16y  = size_type_y<int16_t>;
using offi16    = offset_type<int16_t>;
using offi16x   = offset_type_x<int16_t>;
using offi16y   = offset_type_y<int16_t>;

using recti32 = axis_aligned_rect<int32_t>;
using recti16 = axis_aligned_rect<int16_t>;

} //namespace boken
