//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_DETAIL_TASK_BASE_H
#define MEMORIA_FIBERS_DETAIL_TASK_BASE_H

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/future/detail/shared_state.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {
namespace detail {

template< typename R, typename ... Args >
struct task_base : public shared_state< R > {
    typedef ::boost::intrusive_ptr< task_base >  ptr_type;

    virtual ~task_base() {
    }

    virtual void run( Args && ... args) = 0;

    virtual ptr_type reset() = 0;
};

}}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_DETAIL_TASK_BASE_H
