#include "data.hpp"
#include "item_def.hpp"
#include "entity_def.hpp"
#include "utility.hpp"

namespace boken {

class game_database_impl final : public game_database {
public:
    item_definition const* find(item_id const id) const noexcept final override {
        return find_(item_defs_, id);
    }

    entity_definition const* find(entity_id const id) const noexcept final override {
        return find_(entity_defs_, id);
    }
private:
    template <typename Container, typename Id>
    static auto find_(Container&& c, Id const id) noexcept {
        using namespace container_algorithms;
        return find_if(c, [id](auto const& def) noexcept {
            return id == def.id;
        });
    }

    std::vector<item_definition>   item_defs_;
    std::vector<entity_definition> entity_defs_;
};

std::unique_ptr<game_database> make_game_database() {
    return std::make_unique<game_database_impl>();
}

} //namespace boken
