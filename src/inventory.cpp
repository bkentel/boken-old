#include "inventory.hpp"
#include "text.hpp"

#include "bkassert/assert.hpp"

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

    int16_t header_height() const noexcept final override {
        return static_cast<int16_t>(trender_.line_gap());
    }

    int16_t row_height(int const i) const noexcept final override {
        BK_ASSERT(i > 0 && static_cast<size_t>(i) < rows());
        return header_height();
    }

    int16_t col_width(int const i) const noexcept final override {
        auto const info = col(i);
        return static_cast<int16_t>(info.right - info.left);
    }

    point2i position() const noexcept final override {
        return {100, 100};
    }

    void move_to(point2i const p) noexcept final override {
    }

    recti bounds() const noexcept final override {
        auto const p = position();
        return {p.x, p.y, sizeix {400}, sizeiy {300}};
    }

    void resize_to(sizeix const w, sizeiy const h) noexcept final override {
    }

    size_t size() const noexcept final override {
        return rows();
    }

    size_t rows() const noexcept final override {
        return rows_.size();
    }

    size_t cols() const noexcept final override {
        return col_ids_.size();
    }

    bool empty() const noexcept final override {
        return rows_.empty();
    }

    void add_column(
        uint8_t const id
      , std::string   label
      , get_f         get
      , int const     insert_before
      , int const     width
    ) final override {
        BK_ASSERT((insert_before == insert_at_end)
               || (insert_before >= 0 && insert_before <= cols()));

        BK_ASSERT((width == adjust_to_fit)
               || (width >= 0 && width < std::numeric_limits<int16_t>::max()));

        auto const offset = (insert_before == insert_at_end)
          ? cols()
          : static_cast<size_t>(insert_before);

        auto const w = (width == inventory_list::adjust_to_fit)
          ? std::numeric_limits<int16_t>::max()
          : static_cast<int16_t>(width);

        col_ids_.insert(begin(col_ids_) + offset, id);
        col_get_fs_.insert(begin(col_get_fs_) + offset, std::move(get));
        col_labels_.insert(begin(col_labels_) + offset
          , text_layout {trender_, std::move(label), w});
    }

    void add_row(item_instance_id const id) final override {
        std::vector<text_layout> row;
        row.reserve(cols());

        auto const& itm = lookup_(id);

        for (size_t i = 0; i < cols(); ++i) {
            row.push_back({trender_, col_get_fs_[i](itm)});
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
        BK_ASSERT(index >= 0 && static_cast<size_t>(index) < cols());

        auto const& col = col_labels_[index];

        return {col
              , value_cast<int16_t>(col.position().x)
              , value_cast<int16_t>(col.max_width())
              , col_ids_[index]};
    }

    std::pair<text_layout const*, text_layout const*>
    row(int const index) const noexcept final override {
        BK_ASSERT(index >= 0 && static_cast<size_t>(index) < rows());

        auto const& row = rows_[index];
        return {row.data()
              , row.data() + row.size()};
    }

    void layout() noexcept final override {
        // set column widths
        int32_t x = 0;

        for (size_t i = 0; i < cols(); ++i) {
            auto&      col   = col_labels_[i];
            auto const col_w = col.extent().width();

            col.move_to(x, 0);
            x += col_w;
        }

        // layout rows
        int32_t y = 0;

        for (size_t yi = 0; yi < rows(); ++yi) {
            x =  0;
            y += header_height();
            auto& row = rows_[yi];
            for (size_t xi = 0; xi < cols(); ++xi) {
                auto& cell = row[xi];
                auto const cell_w = cell.extent().width();

                cell.move_to(x, y);
                x += cell_w;
            }
        }
    }
private:
    text_renderer& trender_;
    lookup_f lookup_;
    int indicated_ = 0;
    std::vector<int> selected_;

    std::vector<uint8_t>     col_ids_;
    std::vector<text_layout> col_labels_;
    std::vector<get_f>       col_get_fs_;

    std::vector<std::vector<text_layout>> rows_;
};

std::unique_ptr<inventory_list> make_inventory_list(
    text_renderer&           trender
  , inventory_list::lookup_f lookup
) {
    return std::make_unique<inventory_list_impl>(trender, std::move(lookup));
}

} //namespace boken
