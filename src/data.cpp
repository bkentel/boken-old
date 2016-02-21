#include "data.hpp"
#include "entity_def.hpp"  // for entity_definition
#include "item_def.hpp"    // for item_definition
#include "utility.hpp"     // for find_ptr_if
#include <vector>          // for vector

namespace {
template <typename Container, typename Id>
auto find_(Container&& c, Id const id) noexcept {
    using namespace boken::container_algorithms;
    return find_ptr_if(c, [id](auto const& def) noexcept {
        return id == def.id;
    });
}
} //namespace anonymous

namespace boken {

game_database::~game_database() = default;

class game_database_impl final : public game_database {
public:
    item_definition const* find(item_id const id) const noexcept final override {
        return find_(item_defs_, id);
    }

    entity_definition const* find(entity_id const id) const noexcept final override {
        return find_(entity_defs_, id);
    }
private:
    std::vector<item_definition>   item_defs_;
    std::vector<entity_definition> entity_defs_;
};

std::unique_ptr<game_database> make_game_database() {
    return std::make_unique<game_database_impl>();
}

} //namespace boken
