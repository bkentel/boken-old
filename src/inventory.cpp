#include "inventory.hpp"
#include "text.hpp"
#include "math.hpp"

#include "bkassert/assert.hpp"

#include <algorithm>
#include <vector>

namespace boken {

inventory_list::~inventory_list() = default;

class inventory_list_impl final : public inventory_list {
public:
    explicit inventory_list_impl(text_renderer& trender, lookup_f lookup)
      : trender_ {trender}
      , lookup_ {std::move(lookup)}
    {
    }

    sizei32y header_height() const noexcept final override {
        return trender_.line_gap();
    }

    sizei32y row_height(int const i) const noexcept final override {
        BK_ASSERT(i > 0 && static_cast<size_t>(i) < rows());
        return header_height();
    }

    sizei32x col_width(int const i) const noexcept final override {
        auto const info = col(i);
        return info.right - info.left;
    }

    point2i32 position() const noexcept final override {
        return position_;
    }

    void move_to(point2i32 const p) noexcept final override {
        position_ += (p - position_);
    }

    recti32 bounds() const noexcept final override {
        return {position_, width_, height_};
    }

    void resize_to(sizei32x const w, sizei32y const h) noexcept final override {
        width_  = w;
        height_ = h;
    }

    size_t size() const noexcept final override {
        return rows();
    }

    size_t rows() const noexcept final override {
        return rows_.size();
    }

    size_t cols() const noexcept final override {
        return cols_.size();
    }

    bool empty() const noexcept final override {
        return rows_.empty();
    }

    void add_column(
        uint8_t     id
      , std::string label
      , get_f       get
      , int         insert_before
      , sizei16x    width
    ) final override;

    void add_row(item_instance_id const id) final override {
        row_data row;
        row.reserve(cols());

        auto const& itm = lookup_(id);

        for (size_t i = 0; i < cols(); ++i) {
            row.push_back({trender_, cols_[i].getter(itm), cols_[i].max_width});
        }

        rows_.push_back(std::move(row));
    }

    int indicated() const noexcept final override {
        return indicated_;
    }

    void indicate_next(int const n) final override {
        BK_ASSERT(n >= 0 && indicated_ >= 0);

        indicated_ = static_cast<int>(
            static_cast<size_t>(indicated_ + n) % rows());
    }

    void indicate_prev(int const n) final override {
        BK_ASSERT(n >= 0 && indicated_ >= 0);

        indicated_ = static_cast<int>(rows())
            - static_cast<int>(static_cast<size_t>(n) % rows());
    }

    void selection_set(std::initializer_list<int> const rows) final override {
        selection_clear();
        std::copy_if(begin(rows), end(rows), back_inserter(selected_)
          , [&](int const i) noexcept {
                auto const ok = i >= 0 && i < static_cast<int>(this->rows());
                BK_ASSERT(ok);
                return ok;
            });
        std::sort(begin(selected_), end(selected_));
        auto const last = std::unique(begin(selected_), end(selected_));
        selected_.erase(last, end(selected_));
    }

    void selection_union(std::initializer_list<int> const rows) final override {
        auto const last = end(selected_);
        auto const find = [&](int const i) noexcept {
            return last != std::find(begin(selected_), last, i);
        };

        std::copy_if(begin(rows), end(rows), back_inserter(selected_)
          , [&](int const i) noexcept { return find(i); });

        std::sort(begin(selected_), end(selected_));
    }

    void selection_clear() final override {
        selected_.clear();

    }

    std::pair<int const*, int const*> get_selection() const final override {
        if (selected_.empty()) {
            return {nullptr, nullptr};
        }

        return {selected_.data()
              , selected_.data() + selected_.size()};
    }

    column_info col(int const index) const noexcept final override {
        BK_ASSERT(index >= 0);
        auto const i = static_cast<size_t>(index);
        BK_ASSERT(i < cols());

        auto const& col = cols_[i];
        auto const r = col.text.extent();

        return {col.text
              , underlying_cast_unsafe<int16_t>(r.x0)
              , underlying_cast_unsafe<int16_t>(r.x1)
              , col.min_width
              , col.max_width
              , col.id};
    }

    std::pair<text_layout const*, text_layout const*>
    row(int const index) const noexcept final override {
        BK_ASSERT(index >= 0);
        auto const i = static_cast<size_t>(index);
        BK_ASSERT(i < rows());

        auto const& row = rows_[i];
        return {row.data()
              , row.data() + row.size()};
    }

    void layout() noexcept final override;
private:
    struct col_data {
        text_layout text;
        get_f       getter;
        offi16x     left;
        offi16x     right;
        sizei16x    min_width;
        sizei16x    max_width;
        uint8_t     id;
    };

    using row_data = std::vector<text_layout>;

    text_renderer& trender_;
    lookup_f       lookup_;

    point2i32 position_ {100, 100};
    sizei32x  width_    {400};
    sizei32y  height_   {200};

    int indicated_ {0};
    std::vector<int> selected_;

    std::vector<col_data> cols_;
    std::vector<row_data> rows_;
};

std::unique_ptr<inventory_list> make_inventory_list(
    text_renderer&           trender
  , inventory_list::lookup_f lookup
) {
    return std::make_unique<inventory_list_impl>(trender, std::move(lookup));
}

void inventory_list_impl::add_column(
    uint8_t const  id
  , std::string    label
  , get_f          get
  , int const      insert_before
  , sizei16x const width
) {
    auto const index = [&]() noexcept -> size_t {
        if (insert_before == insert_at_end) {
            return cols();
        }

        BK_ASSERT(insert_before > 0);
        auto const result = static_cast<size_t>(insert_before);
        BK_ASSERT(result <= cols());

        return result;
    }();

    auto const max_w = [&]() noexcept -> sizei16x {
        if (value_cast(width) == adjust_to_fit) {
            return {std::numeric_limits<int16_t>::max()};
        }

        BK_ASSERT(value_cast(width) >= 0);
        return width;
    }();

    auto text = text_layout {trender_, std::move(label), max_w};

    auto const min_w =
        underlying_cast_unsafe<int16_t>(text.extent().width());

    auto const left = (index == 0u)
        ? offi16x {}
        : cols_[index - 1u].right;

    cols_.insert(
        begin(cols_) + static_cast<ptrdiff_t>(index)
      , col_data {
            std::move(text)
          , std::move(get)
          , left
          , left + min_w
          , min_w
          , max_w
          , id});
}

void inventory_list_impl::layout() noexcept {
    auto const header_h = value_cast_unsafe<int16_t>(header_height());

    int16_t x = 0;
    int16_t y = 0;

    auto const get_max_col_w = [&](size_t const i) noexcept {
        auto const header_w = cols_[i].text.extent().width();

        auto const first = begin(rows_);
        auto const last  = end(rows_);

        if (first == last) {
            return header_w;
        }

        auto const& row = *std::max_element(first, last
            , [i](row_data const& lhs, row_data const& rhs) noexcept {
                return lhs[i].extent().width() < rhs[i].extent().width();
            });

        return std::max(header_w, row[i].extent().width());
    };

    // layout column headers
    for (size_t i = 0; i < cols(); ++i) {
        auto& col = cols_[i];

        auto const w = clamp(get_max_col_w(i), col.min_width, col.max_width);

        col.left  = x;
        col.right = col.left + underlying_cast_unsafe<int16_t>(w);

        col.text.move_to(value_cast(col.left), y);

        x += value_cast_unsafe<int16_t>(w);
    }

    x = 0;
    y = 0;

    // resize cells
    for (size_t yi = 0; yi < rows(); ++yi) {
        x =  0;
        y += header_h;

        auto& row = rows_[yi];

        for (size_t xi = 0; xi < cols(); ++xi) {
            auto const& col  = cols_[xi];
            auto&       cell = row[xi];

            cell.move_to(value_cast(col.left), y);
        }
    }
}

} //namespace boken
