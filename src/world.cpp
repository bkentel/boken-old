#include "world.hpp"
#include "level.hpp"           // for level
#include "item.hpp"

#include <algorithm>           // for move
#include <vector>              // for vector

namespace boken {

world::~world() = default;

void item_deleter::operator()(item_instance_id const id) const noexcept {
    world_->free_item(id);
}

class world_impl final : public world {
public:
    world_impl() {
    }

    item const* find(item_instance_id id) const noexcept final override;

    entity const* find(entity_instance_id id) const noexcept final override;

    unique_item create_item(std::function<item (item_instance_id)> f) final override {
        // + 1 as 0 must be reserved to act as a nullptr
        item_instance_id id {static_cast<int>(next_free_item_) + 1};

        if (next_free_item_ < items_.size()) {
            auto const index = *reinterpret_cast<size_t const*>(&items_[next_free_item_]);
            new (&items_[next_free_item_]) item {f(id)};
            next_free_item_ = index;
        } else {
            items_.push_back(f(id));
            ++next_free_item_;
        }

        return unique_item {id, item_deleter_};
    }

    entity_instance_id create_entity_id() final override {
        return entity_instance_id {next_entity_instance_id_++};
    }

    void free_item(item_instance_id const id) final override {
        auto const value = value_cast(id);
        BK_ASSERT(value > 0);

        // - 1 see create_item
        auto const index = static_cast<size_t>(value - 1);
        BK_ASSERT(index < items_.size());

        call_destructor(items_[index]);
        *reinterpret_cast<size_t*>(&items_[index]) = next_free_item_;
        next_free_item_ = index;
    }

    void free_entity(entity_instance_id const id) final override {
        if (value_cast(id) == next_entity_instance_id_ - 1) {
            --next_entity_instance_id_;
        }
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
    item_deleter item_deleter_ {this};

    size_t next_free_item_ {0};
    std::vector<item> items_;

    item_instance_id::type   next_item_instance_id_   {};
    entity_instance_id::type next_entity_instance_id_ {};

    size_t current_level_index_ {0};
    std::vector<std::unique_ptr<level>> levels_;
};

item const* world_impl::find(item_instance_id const id) const noexcept {
    BK_ASSERT(value_cast(id) > 0);
    auto const i = static_cast<size_t>(value_cast(id) - 1);
    return &items_[i];
}

entity const* world_impl::find(entity_instance_id const id) const noexcept {
    for (auto const& lvl : levels_) {
        if (auto const e = lvl->find(id)) {
            return e;
        }
    }

    return nullptr;
}

std::unique_ptr<world> make_world() {
    return std::make_unique<world_impl>();
}

} //namespace boken
