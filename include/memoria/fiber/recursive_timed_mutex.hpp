
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spinlock

#ifndef MEMORIA_FIBERS_RECURSIVE_TIMED_MUTEX_H
#define MEMORIA_FIBERS_RECURSIVE_TIMED_MUTEX_H

#include <chrono>
#include <cstddef>

#include <boost/config.hpp>

#include <boost/assert.hpp>

#include <memoria/fiber/context.hpp>
#include <memoria/fiber/detail/config.hpp>
#include <memoria/fiber/detail/convert.hpp>
#include <memoria/fiber/detail/spinlock.hpp>


#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace memoria {
namespace fibers {

class condition_variable;

class MEMORIA_FIBERS_DECL recursive_timed_mutex {
private:
    friend class condition_variable;

    typedef context::wait_queue_t   wait_queue_type;

    detail::spinlock            wait_queue_splk_{};
    wait_queue_type             wait_queue_{};
    context                 *   owner_{ nullptr };
    std::size_t                 count_{ 0 };

    bool try_lock_until_( std::chrono::steady_clock::time_point const& timeout_time) noexcept;

public:
    recursive_timed_mutex() = default;

    ~recursive_timed_mutex() {
        BOOST_ASSERT( nullptr == owner_);
        BOOST_ASSERT( 0 == count_);
        BOOST_ASSERT( wait_queue_.empty() );
    }

    recursive_timed_mutex( recursive_timed_mutex const&) = delete;
    recursive_timed_mutex & operator=( recursive_timed_mutex const&) = delete;

    void lock();

    bool try_lock() noexcept;

    template< typename Clock, typename Duration >
    bool try_lock_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        return try_lock_until_( timeout_time);
    }

    template< typename Rep, typename Period >
    bool try_lock_for( std::chrono::duration< Rep, Period > const& timeout_duration) {
        return try_lock_until_( std::chrono::steady_clock::now() + timeout_duration);
    }

    void unlock();
};

}}

#ifdef _MSC_VER
# pragma warning(pop)
#endif


#endif // MEMORIA_FIBERS_RECURSIVE_TIMED_MUTEX_H
