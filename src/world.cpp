#include "world.hpp"
#include "level.hpp"           // for level
#include "item.hpp"
#include "entity.hpp"

#include <algorithm>           // for move
#include <vector>              // for vector

namespace boken {

template <typename T>
class contiguous_fixed_size_block_storage {
public:
    static_assert(sizeof(T) >= sizeof(size_t), "");

    size_t next_block_id() const noexcept {
        return next_free_ + 1; // ids start at 1
    }

    template <typename... Args>
    std::pair<T*, size_t> allocate(Args&&... args) {
        if (next_free_ >= data_.size()) {
            data_.push_back(T {std::forward<Args>(args)...});
            return {std::addressof(data_.back()), ++next_free_}; // ids start at 1
        }

        auto*      p = data_.data() + next_free_;
        auto const i = *reinterpret_cast<size_t const*>(p);

        new (p) T {std::forward<Args>(args)...};

        return {p, (next_free_ = i) + 1}; // ids start at 1
    }

    void deallocate(size_t const i) noexcept {
        // ids start at 1
        BK_ASSERT(i > 0);
        BK_ASSERT(i < data_.size() + 1);

        auto const index = i - 1;
        call_destructor(data_[index]);

        *reinterpret_cast<size_t*>(&data_[index]) = next_free_;
        next_free_ = index;
    }

    size_t capacity() const noexcept { return data_.size(); }

    T&       operator[](size_t const i)       noexcept { return data_[i - 1]; }
    T const& operator[](size_t const i) const noexcept { return data_[i - 1]; }
private:
    size_t         next_free_ {0};
    std::vector<T> data_;
};

world::~world() = default;

class world_impl final : public world {
    friend item_deleter;
    friend entity_deleter;
public:
    world_impl() {
    }

    item* find(item_instance_id id) noexcept final override;
    entity* find(entity_instance_id id) noexcept final override;

    item const* find(item_instance_id const id) const noexcept final override {
        return const_cast<world_impl*>(this)->find(id);
    }

    entity const* find(entity_instance_id const id) const noexcept final override {
        return const_cast<world_impl*>(this)->find(id);
    }

    unique_item create_item(std::function<item (item_instance_id)> const& f) final override {
        auto const id = item_instance_id {static_cast<int32_t>(items_.next_block_id())};
        auto const result = items_.allocate(f(id));

        BK_ASSERT(value_cast<size_t>(id) == result.second);

        return unique_item {id, item_deleter_};
    }
    unique_entity create_entity(std::function<entity (entity_instance_id)> const& f) final override {
        auto const id = entity_instance_id {static_cast<int32_t>(entities_.next_block_id())};
        auto const result = entities_.allocate(f(id));

        BK_ASSERT(value_cast<size_t>(id) == result.second);

        return unique_entity {id, entity_deleter_};
    }

    int total_levels() const noexcept final override {
        return static_cast<int>(levels_.size());
    }

    level& current_level() noexcept final override {
        return *levels_[current_level_index_];
    }

    level const& current_level() const noexcept final override {
        return *levels_[current_level_index_];
    }

    level& add_new_level(level* parent, std::unique_ptr<level> level) final override {
        levels_.push_back(std::move(level));
        return *levels_.back();
    }
private:
    item_deleter   item_deleter_   {this};
    entity_deleter entity_deleter_ {this};

    contiguous_fixed_size_block_storage<item>   items_;
    contiguous_fixed_size_block_storage<entity> entities_;

    size_t current_level_index_ {0};
    std::vector<std::unique_ptr<level>> levels_;
};

void item_deleter::operator()(item_instance_id const id) const noexcept {
    static_cast<world_impl*>(world_)->items_.deallocate(value_cast<size_t>(id));
}

void entity_deleter::operator()(entity_instance_id const id) const noexcept {
    static_cast<world_impl*>(world_)->entities_.deallocate(value_cast<size_t>(id));
}

item* world_impl::find(item_instance_id const id) noexcept {
    auto const i = value_cast<size_t>(id);
    return ((i < 1) || (i > items_.capacity()))
      ? nullptr
      : &items_[i];
}

entity* world_impl::find(entity_instance_id const id) noexcept {
    auto const i = value_cast<size_t>(id);
    return ((i < 1) || (i > entities_.capacity()))
      ? nullptr
      : &entities_[i];
}

entity const* find(world const& w, entity_instance_id const id) noexcept {
    return w.find(id);
}

item const* find(world const& w, item_instance_id const id) noexcept {
    return w.find(id);
}

std::unique_ptr<world> make_world() {
    return std::make_unique<world_impl>();
}

} //namespace boken
