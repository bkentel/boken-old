#include "world.hpp"
#include "level.hpp"           // for level

#include <algorithm>           // for move
#include <vector>              // for vector

namespace boken {

world::~world() = default;

class world_impl final : public world {
public:
    world_impl() {
    }

    item const* find(item_instance_id id) const noexcept final override;

    entity const* find(entity_instance_id id) const noexcept final override;

    item_instance_id create_item_id() final override {
        return item_instance_id {next_item_instance_id_++};
    }

    entity_instance_id create_entity_id() final override {
        return entity_instance_id {next_entity_instance_id_++};
    }

    void free_item_id(item_instance_id const id) final override {
        if (value_cast(id) == next_item_instance_id_ - 1) {
            --next_item_instance_id_;
        }
    }

    void free_entity_id(entity_instance_id const id) final override {
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
    item_instance_id::type   next_item_instance_id_   {};
    entity_instance_id::type next_entity_instance_id_ {};

    size_t current_level_index_ {0};
    std::vector<std::unique_ptr<level>> levels_;
};

item const* world_impl::find(item_instance_id const id) const noexcept {
    for (auto const& lvl : levels_) {
        if (auto const i = lvl->find(id)) {
            return i;
        }
    }
    return nullptr;
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
