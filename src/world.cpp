#include "world.hpp"
#include "level.hpp"

namespace boken {

world::~world() = default;

class world_impl final : public world {
public:
    world_impl() {
    }

    item const* find(item_instance_id id) const noexcept final override {
        return nullptr;
    }

    entity const* find(entity_instance_id id) const noexcept final override {
        return nullptr;
    }

    item_instance_id create_item_id() final override {
        return item_instance_id{next_item_instance_id_++};
    }

    entity_instance_id create_entity_id() final override {
        return entity_instance_id{next_entity_instance_id_++};
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

    int current_level_index_ {0};
    std::vector<std::unique_ptr<level>> levels_;
};

std::unique_ptr<world> make_world() {
    return std::make_unique<world_impl>();
}

} //namespace boken
