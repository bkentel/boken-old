#pragma once

#include "types.hpp"
#include "config.hpp"
#include "math_types.hpp"

#include <string>
#include <functional>
#include <initializer_list>
#include <memory>
#include <cstdint>

namespace boken { class text_renderer; }
namespace boken { class text_layout; }
namespace boken { class item; }

namespace boken {
class inventory_list {
public:
    //! function used to lookup an actual item instance from an id.
    using lookup_f = std::function<item const& (item_instance_id id)>;

    //! function used to get the text for a cell from an item instance.
    using get_f = std::function<std::string (item const& itm)>;

    //! insert new column of row and the end
    static int const insert_at_end = -1;

    //! use a dynamically adjustable width for the column in lieu of static.
    static int16_t const adjust_to_fit = -1;

    //--------------------------------------------------------------------------
    virtual ~inventory_list();

    //--------------------------------------------------------------------------
    virtual sizei32y header_height() const noexcept = 0;

    virtual sizei32y row_height(int i) const noexcept = 0;
    virtual sizei32x col_width(int i) const noexcept = 0;

    virtual point2i32 position() const noexcept = 0;
    virtual recti32   bounds()   const noexcept = 0;

    //virtual point2i32 client_position() const noexcept = 0;
    //virtual recti32   client_bounds()   const noexcept = 0;

    //--------------------------------------------------------------------------
    virtual size_t size()  const noexcept = 0;
    virtual bool   empty() const noexcept = 0;

    virtual size_t rows() const noexcept = 0;
    virtual size_t cols() const noexcept = 0;

    //--------------------------------------------------------------------------
    virtual void resize_to(sizei32x w, sizei32y h) noexcept = 0;
    virtual void move_to(point2i32 p) noexcept = 0;
    virtual void move_by(vec2i32 v) noexcept = 0;

    //--------------------------------------------------------------------------
    struct hit_test_result {
        enum class type : uint32_t {
            none   //!< no hit
          , header //!< column header
          , cell   //!< table cell
          , title  //!< window title
          , frame  //!< window frame
        };

        explicit operator bool() const noexcept { return what != type::none; }

        type    what;
        int32_t x;
        int32_t y;
    };

    virtual hit_test_result cell_at(point2i32 p) const noexcept = 0;

    //--------------------------------------------------------------------------
    virtual int  indicated() const noexcept = 0;
    virtual void indicate_next(int n = 1) = 0;
    virtual void indicate_prev(int n = 1) = 0;

    //--------------------------------------------------------------------------
    virtual void reserve(size_t cols, size_t rows) = 0;

    virtual void add_column(
        uint8_t     id
      , std::string label
      , get_f       get
      , int         insert_before = insert_at_end
      , sizei16x    width         = adjust_to_fit) = 0;

    virtual void add_row(item_instance_id id) = 0;

    //--------------------------------------------------------------------------
    virtual void selection_set(std::initializer_list<int> rows) = 0;
    virtual void selection_union(std::initializer_list<int> rows) = 0;
    virtual void selection_clear() = 0;
    virtual std::pair<int const*, int const*> get_selection() const = 0;

    //--------------------------------------------------------------------------
    struct column_info {
        text_layout const& text;
        offi16x  left;
        offi16x  right;
        sizei16x min_width;
        sizei16x max_width;
        uint8_t  id;
    };

    virtual column_info col(int index) const noexcept = 0;

    //--------------------------------------------------------------------------
    virtual std::pair<text_layout const*, text_layout const*>
        row(int index) const noexcept = 0;

    //--------------------------------------------------------------------------
    virtual void layout() noexcept = 0;
};

std::unique_ptr<inventory_list> make_inventory_list(
    text_renderer&           trender
  , inventory_list::lookup_f lookup
);


} //namespace boken
