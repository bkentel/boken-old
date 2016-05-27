#pragma once

#include "types.hpp"
#include <memory>
#include <functional>

namespace boken { class item; }
namespace boken { class entity; }
namespace boken { class level; }
namespace boken { class item_deleter; }
namespace boken { class entity_deleter; }

namespace boken {

//=====--------------------------------------------------------------------=====
// All state associated with the game world as a whole.
//=====--------------------------------------------------------------------=====
class world {
public:
    virtual ~world();

    //@{
    //! @returns The instance associated with a given @p id.
    //! @pre     The @p id must be valid.
    //! @note    The reference returned may be invalidated by a call to
    //!          @ref create_object.

    virtual item   const& find(item_instance_id   id) const noexcept = 0;
    virtual entity const& find(entity_instance_id id) const noexcept = 0;
    virtual item&         find(item_instance_id   id)       noexcept = 0;
    virtual entity&       find(entity_instance_id id)       noexcept = 0;

    //@}

    virtual item_deleter   const& get_item_deleter()   const noexcept = 0;
    virtual entity_deleter const& get_entity_deleter() const noexcept = 0;

    //@{
    //! @returns An owning handle to a new object created by the functor @p f.
    //! @note    References returned by @ref find can be invalidated by a call
    //!          to this function.

    virtual unique_item   create_object(std::function<item   (item_instance_id)>   const& f) = 0;
    virtual unique_entity create_object(std::function<entity (entity_instance_id)> const& f) = 0;

    //@}

    virtual int total_levels() const noexcept = 0;

    virtual level&       current_level()       noexcept = 0;
    virtual level const& current_level() const noexcept = 0;

    virtual bool   has_level(size_t const id) const noexcept = 0;
    virtual level& add_new_level(level* parent, std::unique_ptr<level> level) = 0;
    virtual level& change_level(size_t const id) = 0;
};

std::unique_ptr<world> make_world();

} //namespace boken
