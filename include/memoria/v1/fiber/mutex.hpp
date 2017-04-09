
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/config.hpp>

#include <boost/assert.hpp>

#include <memoria/v1/fiber/context.hpp>
#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace memoria {
namespace v1 {    
namespace fibers {

class condition_variable;

class MEMORIA_V1_FIBERS_DECL mutex {
private:
    friend class condition_variable;

    typedef context::wait_queue_t   wait_queue_t;

    context                 *   owner_{ nullptr };
    wait_queue_t                wait_queue_{};
    detail::spinlock            wait_queue_splk_{};

public:
    mutex() = default;

    ~mutex() {
        BOOST_ASSERT( nullptr == owner_);
        BOOST_ASSERT( wait_queue_.empty() );
    }

    mutex( mutex const&) = delete;
    mutex & operator=( mutex const&) = delete;

    void lock();

    bool try_lock();

    void unlock();
};

}}}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif


