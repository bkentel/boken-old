#include "world.hpp"
#include "level.hpp"           // for level
#include "item.hpp"
#include "entity.hpp"
#include "allocator.hpp"

#include <algorithm>           // for move
#include <vector>              // for vector

namespace boken {

world::~world() = default;

class world_impl final : public world {
    friend item_deleter;
    friend entity_deleter;
public:
    world_impl() {
    }

    item& find(item_instance_id id) noexcept final override;
    entity& find(entity_instance_id id) noexcept final override;

    item const& find(item_instance_id const id) const noexcept final override {
        return const_cast<world_impl*>(this)->find(id);
    }

    entity const& find(entity_instance_id const id) const noexcept final override {
        return const_cast<world_impl*>(this)->find(id);
    }

    unique_item create_object(std::function<item (item_instance_id)> const& f) final override {
        auto const id = item_instance_id {static_cast<uint32_t>(items_.next_block_id())};
        auto const result = items_.allocate(f(id));

        BK_ASSERT(value_cast<size_t>(id) == result.second);

        return unique_item {id, item_deleter_};
    }
    unique_entity create_object(std::function<entity (entity_instance_id)> const& f) final override {
        auto const id = entity_instance_id {static_cast<uint32_t>(entities_.next_block_id())};
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

    bool has_level(size_t const id) const noexcept final override {
        auto const it = std::find_if(begin(levels_), end(levels_)
          , [&](auto const& lvl) noexcept { return lvl->id() == id; });

        return it != end(levels_);
    }

    level& add_new_level(level* parent, std::unique_ptr<level> level) final override {
        levels_.push_back(std::move(level));
        return *levels_.back();
    }

    level& change_level(size_t const id) final override {
        if (id < levels_.size()) {
            current_level_index_ = id;
        }

        return current_level();
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

namespace {

template <typename Container, typename Id>
auto&& find_impl_(Container&& c, Id const id) noexcept {
    auto const i = value_cast(id);
    static_assert(std::is_unsigned<decltype(i)>::value, "");

    BK_ASSERT((i >= 1u) && (i <= c.capacity()));

    return c[i];
}

} // namespace

item& world_impl::find(item_instance_id const id) noexcept {
    return find_impl_(items_, id);
}

entity& world_impl::find(entity_instance_id const id) noexcept {
    return find_impl_(entities_, id);
}

std::unique_ptr<world> make_world() {
    return std::make_unique<world_impl>();
}

item& find(world& w, item_instance_id const id) noexcept {
    return w.find(id);
}

entity& find(world& w, entity_instance_id const id) noexcept {
    return w.find(id);
}

item const& find(world const& w, item_instance_id const id) noexcept {
    return w.find(id);
}

entity const& find(world const& w, entity_instance_id const id) noexcept {
    return w.find(id);
}

unique_item create_object(world& w, std::function<item (item_instance_id)> const& f) {
    return w.create_object(f);
}

unique_entity create_object(world& w, std::function<entity (entity_instance_id)> const& f) {
    return w.create_object(f);
}

string_view name_of(world const& w, game_database const& db, item_instance_id const id) noexcept {
    return name_of(db, w.find(id));
}

string_view name_of(world const& w, game_database const& db, entity_instance_id const id) noexcept {
    return name_of(db, w.find(id));
}

} //namespace boken
