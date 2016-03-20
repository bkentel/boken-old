#pragma once

#include "types.hpp"
#include "config.hpp"
#include "math.hpp"

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
    using lookup_f = std::function<item const& (item_instance_id id)>;
    using get_f    = std::function<std::string (item const& itm)>;

    static constexpr int insert_at_end = -1;
    static constexpr int adjust_to_fit = -1;

    struct column_info {
        text_layout const&     text;
        offset_type_x<int16_t> left;
        offset_type_x<int16_t> right;
        size_type_x<int16_t>   min_width;
        size_type_x<int16_t>   max_width;
        uint8_t                id;
    };

    virtual ~inventory_list();

    virtual int16_t header_height() const noexcept = 0;
    virtual int16_t row_height(int i) const noexcept = 0;
    virtual int16_t col_width(int i) const noexcept = 0;

    virtual point2i position() const noexcept = 0;
    virtual void move_to(point2i p) noexcept = 0;

    virtual recti bounds() const noexcept = 0;
    virtual void resize_to(sizeix w, sizeiy h) noexcept = 0;

    virtual size_t size() const noexcept = 0;
    virtual size_t rows() const noexcept = 0;
    virtual size_t cols() const noexcept = 0;
    virtual bool empty() const noexcept = 0;

    virtual void add_column(
        uint8_t     id
      , std::string label
      , get_f       get
      , int         insert_before = insert_at_end
      , int         width         = adjust_to_fit) = 0;

    virtual void add_row(item_instance_id id) = 0;

    virtual int  indicated() const noexcept = 0;
    virtual void indicate_next(int n = 1) = 0;
    virtual void indicate_prev(int n = 1) = 0;

    virtual void selection_set(std::initializer_list<int> rows) = 0;
    virtual void selection_union(std::initializer_list<int> rows) = 0;
    virtual void selection_clear() = 0;
    virtual std::pair<int const*, int const*> get_selection() const = 0;

    virtual column_info col(int index) const noexcept = 0;
    virtual std::pair<text_layout const*, text_layout const*>
        row(int index) const noexcept = 0;

    virtual void layout() noexcept = 0;
};

std::unique_ptr<inventory_list> make_inventory_list(
    text_renderer&           trender
  , inventory_list::lookup_f lookup
);


} //namespace boken
