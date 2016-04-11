#include "inventory.hpp"
#include "text.hpp"
#include "math.hpp"
#include "rect.hpp"

#include "bkassert/assert.hpp"

#include <algorithm>
#include <vector>

namespace {

template <typename It, typename Predicate>
static ptrdiff_t distance_to_matching_or(
    It const first, It const last, ptrdiff_t const fallback, Predicate pred
) noexcept {
    auto const it = std::find_if(first, last, pred);
    return (it == last) ? fallback : std::distance(first, it);
}

template <typename Container, typename Predicate>
static ptrdiff_t distance_to_matching_or(
    Container&& c, ptrdiff_t const fallback, Predicate pred
) noexcept {
    using std::begin;
    using std::end;
    return distance_to_matching_or(begin(c), end(c), fallback, pred);
}

} // namespace

namespace boken {

inventory_list::~inventory_list() = default;

class inventory_list_impl final : public inventory_list {
public:
    struct layout_config {
        sizei16x frame_w = int16_t {4};
        sizei16y frame_h = int16_t {4};

        sizei16y client_off_y = int16_t {4};

        sizei16x scroll_bar_w = int16_t {16};
        sizei16y scroll_bar_h = int16_t {16};

        sizei16x col_padding = int16_t {3};
        sizei16y row_padding = int16_t {3};
    };

    //--------------------------------------------------------------------------
    explicit inventory_list_impl(text_renderer& trender, lookup_f lookup)
      : trender_ {trender}
      , lookup_ {std::move(lookup)}
      , title_ {trender, "Inventory"}
    {
        move_to({100, 100});
        resize_to(500, 300);
    }

    //--------------------------------------------------------------------------
    void set_title(std::string title) final override {
        title_.layout(trender_, std::move(title));
    }

    //--------------------------------------------------------------------------
    text_layout const& title() const noexcept final override {
        return title_;
    }

    layout_metrics metrics() const noexcept final override {
        return metrics_;
    }

    vec2i32 scroll_offset() const noexcept final override {
        return {0, 0};
    }

    //--------------------------------------------------------------------------
    bool show() noexcept final override {
        bool const result = is_visible_;
        is_visible_ = true;
        return result;
    }

    bool hide() noexcept final override {
        bool const result = is_visible_;
        is_visible_ = false;
        return result;
    }

    bool is_visible() const noexcept final override {
        return is_visible_;
    }

    bool toggle_visible() noexcept final override {
        bool const result = is_visible_;
        is_visible_ = !is_visible_;
        return result;
    }

    //--------------------------------------------------------------------------
    size_t size() const noexcept final override {
        return rows();
    }

    bool empty() const noexcept final override {
        return rows_.empty();
    }

    size_t rows() const noexcept final override {
        return rows_.size();
    }

    size_t cols() const noexcept final override {
        return cols_.size();
    }

    //--------------------------------------------------------------------------
    void resize_to(sizei32x const w, sizei32y const h) noexcept final override {
        auto&       m = metrics_;
        auto const& c = config_;

        auto const title_w = title_.extent().width();
        auto const title_h = title_.extent().height();

        auto const min_w = title_w + c.frame_w * 2;
        auto const min_h = title_h + c.client_off_y + c.frame_h * 2 + sizei32y {1};

        auto const real_w = (w <= min_w)
          ? min_w
          : w;

        auto const real_h = (h <= min_h)
          ? min_h
          : h;

        // outer frame
        m.frame.x1 = m.frame.x0 + real_w;
        m.frame.y1 = m.frame.y0 + real_h;

        // title bar
        m.title.x0 = m.frame.x0 + c.frame_w;
        m.title.x1 = m.frame.x1 - c.frame_w;
        m.title.y0 = m.frame.y0 + c.frame_h;
        m.title.y1 = m.title.y0 + title_.extent().height();

        // client frame
        m.client_frame.x0 = m.frame.x0 + c.frame_w;
        m.client_frame.x1 = m.frame.x1 - c.frame_w;
        m.client_frame.y0 = m.title.y1 + c.client_off_y;
        m.client_frame.y1 = m.frame.y1 - c.frame_h;
    }

    void resize_by(sizei32x const dw, sizei32y const dh, int const side_x, int const side_y) noexcept final override {
        auto const real_dw = dw * (side_x < 0 ? -1 : side_x > 0 ? 1 : 0);
        auto const real_dh = dh * (side_y < 0 ? -1 : side_y > 0 ? 1 : 0);

        auto const w = metrics_.frame.width();
        auto const h = metrics_.frame.height();

        resize_to(w + real_dw, h + real_dh);

        auto const w_after = metrics_.frame.width();
        auto const h_after = metrics_.frame.height();

        auto const v = vec2i32 {
            (side_x < 0) ? -value_cast(w_after - w) : 0
          , (side_y < 0) ? -value_cast(h_after - h) : 0
        };

        move_by(v);
    }

    void move_to(point2i32 const p) noexcept final override {
        move_by(p - metrics_.frame.top_left());
    }

    void move_by(vec2i32 const v) noexcept final override {
        auto& m = metrics_;

        m.frame        += v;
        m.client_frame += v;
        m.title        += v;
        m.close_button += v;
        m.scroll_bar_v += v;
        m.scroll_bar_h += v;
    }

    //--------------------------------------------------------------------------
    hit_test_result hit_test(point2i32 const p0) const noexcept final override;

    //--------------------------------------------------------------------------
    int indicated() const noexcept final override {
        return indicated_;
    }

    void indicate(int const n) noexcept final override {
        BK_ASSERT(n >= 0 && static_cast<size_t>(n) <= rows());
        indicated_ = n;
    }

    void indicate_change_(int const n) noexcept {
        auto const n_rows = rows();

        BK_ASSERT(indicated_ >= 0);
        auto const i = static_cast<size_t>(indicated_);
        BK_ASSERT(i <= n_rows);

        if (n_rows <= 0) {
            indicated_ = 0;
            return;
        }

        auto const m = n < 0
            ? (n_rows - static_cast<size_t>(-n) % n_rows)
            : (static_cast<size_t>(n) % n_rows);

        indicated_ = static_cast<int>((i + m) % n_rows);
    }

    void indicate_next(int const n) noexcept final override {
        indicate_change_(n);
    }

    void indicate_prev(int const n) noexcept final override {
        indicate_change_(-n);
    }

    //--------------------------------------------------------------------------
    void reserve(size_t const cols, size_t const rows) final override {
        cols_.reserve(cols);
        rows_.reserve(rows);
    }

    void add_column(
        uint8_t     id
      , std::string label
      , get_f       get
      , int         insert_before
      , sizei16x    width
    ) final override;

    void add_row(item_instance_id const id) final override {
        add_rows(&id, &id + 1);
    }

    void add_rows(item_instance_id const* const first, item_instance_id const* const last) final override {
        BK_ASSERT(!!first && !!last);

        auto const first_col = begin(cols_);
        auto const last_col  = end(cols_);
        auto const n_cols    = cols();

        auto const make_row = [&](item_instance_id const id) {
            row_t row;
            row.reserve(n_cols);

            auto const& itm = lookup_(id);

            std::transform(first_col, last_col, back_inserter(row)
              , [&](col_data const& col) -> text_layout {
                    return {trender_, col.getter(itm), col.max_width, sizei16y {}};
                });

            return row;
        };

        std::transform(first, last, back_inserter(rows_), make_row);
        std::copy(first, last, back_inserter(row_data_));
    }

    void remove_row(int const i) noexcept final override {
        remove_rows(&i, &i + 1);
    }

    void remove_rows(int const* const first, int const* const last) noexcept final override {
        BK_ASSERT(!!first && !!last);

        auto const n_rows = rows();

        std::for_each(first, last, [&](int const i) noexcept {
            BK_ASSERT(i >= 0);
            auto const j = static_cast<size_t>(i);
            BK_ASSERT(j < n_rows);

            rows_[j].clear();
            row_data_[j] = item_instance_id {};
        });

        rows_.erase(std::remove_if(begin(rows_), end(rows_)
          , [](auto const& row) noexcept { return row.empty(); }));

        row_data_.erase(std::remove_if(begin(row_data_), end(row_data_)
          , [](auto const id) noexcept { return value_cast(id) == 0; }));
    }

    void clear_rows() noexcept final override {
        rows_.clear();
        row_data_.clear();
        selected_.clear();
        indicated_ = 0;
    }

    void clear() noexcept final override {
        clear_rows();
        cols_.clear();
    }

    //--------------------------------------------------------------------------
    bool selection_toggle(int const row) final override {
        BK_ASSERT(row >= 0 && static_cast<size_t>(row) < rows());

        auto const it = std::find(begin(selected_), end(selected_), row);
        if (it != end(selected_)) {
            selected_.erase(it);
            return false;
        }

        selected_.push_back(row);
        std::sort(begin(selected_), end(selected_));

        return true;
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
          , [&](int const i) noexcept { return !find(i); });

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

    bool is_selected(int const row) const noexcept final override {
        return std::any_of(begin(selected_), end(selected_)
          , [row](int const r) noexcept { return row == r; });
    }

    column_info col(int const index) const noexcept final override {
        BK_ASSERT(index >= 0);
        auto const i = static_cast<size_t>(index);
        BK_ASSERT(i < cols());

        auto const& col = cols_[i];
        auto const r = col.text.extent();

        return {col.text
              , col.min_width
              , col.max_width
              , col.right - col.left
              , col.id};
    }

    //--------------------------------------------------------------------------
    std::pair<text_layout const*, text_layout const*>
    row(int const index) const noexcept final override {
        BK_ASSERT(index >= 0);
        auto const i = static_cast<size_t>(index);
        BK_ASSERT(i < rows());

        auto const& row = rows_[i];
        return {row.data()
              , row.data() + row.size()};
    }

    item_instance_id row_data(int index) const noexcept final override {
        BK_ASSERT(index >= 0);
        auto const i = static_cast<size_t>(index);
        BK_ASSERT(i < rows());

        return row_data_[i];
    }

    //--------------------------------------------------------------------------
    void layout() noexcept final override;
private:

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

    using row_t = std::vector<text_layout>;

    text_renderer& trender_;
    lookup_f       lookup_;

    layout_config  config_;
    layout_metrics metrics_;

    text_layout title_;

    int indicated_ {0};
    std::vector<int> selected_;

    std::vector<col_data>         cols_;
    std::vector<row_t>            rows_;
    std::vector<item_instance_id> row_data_;

    bool is_visible_ = true;
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

    auto text = text_layout {trender_, std::move(label), max_w, sizei16y {}};

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

inventory_list::hit_test_result
inventory_list_impl::hit_test(point2i32 const p0) const noexcept {
    auto const& m = metrics_;
    auto const& c = config_;

    using type = hit_test_result::type;

    // no hit at all
    if (!intersects(m.frame, p0)) {
        return {type::none, -1, -1};
    }

    // check the frame border
    if (!intersects(shrink_rect(m.frame, value_cast(c.frame_w)), p0)) {
        int32_t const x = (abs(m.frame.x0 - p0.x) <= c.frame_w) ? -1
                        : (abs(m.frame.x1 - p0.x) <= c.frame_w) ?  1
                                                                :  0;

        int32_t const y = (abs(m.frame.y0 - p0.y) <= c.frame_h) ? -1
                        : (abs(m.frame.y1 - p0.y) <= c.frame_h) ?  1
                                                                :  0;
        return {type::frame, x, y};
    }

    // a hit, but not inside the client area
    if (!intersects(m.client_frame, p0)) {
        if (intersects(m.title, p0)) {
            return {type::title, 0, 0};
        } else if (intersects(m.close_button, p0)) {
            return {type::button_close, 0, 0};
        } else if (intersects(m.scroll_bar_v, p0)) {
            return {type::scroll_bar_v, 0, 0};
        } else if (intersects(m.scroll_bar_h, p0)) {
            return {type::scroll_bar_h, 0, 0};
        } else {
            return {type::empty, 0, 0};
        }
    }

    //
    // a hit inside the client area
    //

    if (cols() <= 0) {
        return {type::empty, 0, 0};
    }

    // a point relative to the client area
    auto const p = point2i32 {} + (p0 - m.client_frame.top_left()) + scroll_offset();

    auto const col_i = static_cast<int32_t>(distance_to_matching_or(cols_, -1
        , [p](col_data const& col) noexcept {
            return p.x >= col.left && p.x < col.right;
        }));

    auto const row_i = static_cast<int32_t>(distance_to_matching_or(rows_, -1
        , [p](row_t const& row) noexcept {
            if (row.empty()) {
                return false;
            }

            auto const extent = row[0].extent();
            return p.y >= extent.y0 && p.y < extent.y1;
        }));

    // a hit in the column header
    if (row_i < 0) {
        return {type::header, col_i, 0};
    }

    // a hit in a cell
    return {type::cell, col_i, row_i};
}

void inventory_list_impl::layout() noexcept {
    auto const& c = config_;
    auto const& m = metrics_;

    auto const get_max_col_w = [&](size_t const i) noexcept {
        auto const header_w = cols_[i].text.extent().width();

        auto const first = begin(rows_);
        auto const last  = end(rows_);

        if (first == last) {
            return header_w;
        }

        auto const& row = *std::max_element(first, last
            , [i](row_t const& lhs, row_t const& rhs) noexcept {
                return lhs[i].extent().width() < rhs[i].extent().width();
            });

        return std::max(header_w, row[i].extent().width());
    };

    int32_t x = 0;
    int32_t header_h = 0;

    // layout column headers
    for (size_t i = 0; i < cols(); ++i) {
        auto& col = cols_[i];

        auto const w = clamp(get_max_col_w(i), col.min_width, col.max_width);
        auto const h = col.text.extent().height();

        col.left  = static_cast<int16_t>(x);
        col.right = col.left + underlying_cast_unsafe<int16_t>(w) + c.col_padding;

        col.text.move_to(value_cast(col.left), 0);

        x = value_cast(col.right);
        header_h = std::max(header_h, value_cast(h));
    }

    metrics_.header_h = header_h;
    int32_t y = header_h;

    // resize cells
    for (size_t yi = 0; yi < rows(); ++yi) {
        int32_t max_h = 0;
        x = 0;

        auto& row = rows_[yi];

        for (size_t xi = 0; xi < cols(); ++xi) {
            auto const& col  = cols_[xi];
            auto&       cell = row[xi];

            auto const h = cell.extent().height();
            max_h = std::max(max_h, value_cast(h));
            cell.move_to(value_cast(col.left), y);
        }

        y += max_h;
    }
}

} //namespace boken
