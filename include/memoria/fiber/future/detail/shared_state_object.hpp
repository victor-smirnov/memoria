
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_DETAIL_SHARED_STATE_OBJECT_H
#define MEMORIA_FIBERS_DETAIL_SHARED_STATE_OBJECT_H

#include <memory>

#include <boost/config.hpp>

#include <memoria/fiber/detail/config.hpp>
#include <memoria/fiber/future/detail/shared_state.hpp>

namespace memoria {
namespace fibers {
namespace detail {

template< typename R, typename Allocator >
class shared_state_object : public shared_state< R > {
public:
    typedef typename std::allocator_traits< Allocator >::template rebind_alloc< 
        shared_state_object
    >                                           allocator_type;

    shared_state_object( allocator_type const& alloc) :
        shared_state< R >{},
        alloc_{ alloc } {
    }

protected:
    void deallocate_future() noexcept override final {
        destroy_( alloc_, this);
    }

private:
    allocator_type             alloc_;

    static void destroy_( allocator_type const& alloc, shared_state_object * p) noexcept {
        allocator_type a{ alloc };
        typedef std::allocator_traits< allocator_type >    traity_type;
        traity_type::destroy( a, p);
        traity_type::deallocate( a, p, 1);
    }
};

}}}

#endif // MEMORIA_FIBERS_DETAIL_SHARED_STATE_OBJECT_H
