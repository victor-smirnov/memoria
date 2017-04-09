//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>

#include <boost/config.hpp> 

#include <memoria/v1/fiber/context.hpp>
#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/detail/convert.hpp>
#include <memoria/v1/fiber/fiber.hpp>
#include <memoria/v1/fiber/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace v1 {    
namespace this_fiber {

inline
fibers::fiber::id get_id() noexcept {
    return fibers::context::active()->get_id();
}

inline
void yield() noexcept {
    fibers::context::active()->yield();
}

template< typename Clock, typename Duration >
void sleep_until( std::chrono::time_point< Clock, Duration > const& sleep_time_) {
    std::chrono::steady_clock::time_point sleep_time(
            memoria::v1::fibers::detail::convert( sleep_time_) );
    fibers::context::active()->wait_until( sleep_time);
}

template< typename Rep, typename Period >
void sleep_for( std::chrono::duration< Rep, Period > const& timeout_duration) {
    fibers::context::active()->wait_until(
            std::chrono::steady_clock::now() + timeout_duration);
}



template< typename PROPS >
PROPS & properties() {
    fibers::fiber_properties * props =
        fibers::context::active()->get_properties();
    if ( ! props) {
        // props could be nullptr if the thread's main fiber has not yet
        // yielded (not yet passed through algorithm_with_properties::
        // awakened()). Address that by yielding right now.
        yield();
        // Try again to obtain the fiber_properties subclass instance ptr.
        // Walk through the whole chain again because who knows WHAT might
        // have happened while we were yielding!
        props = fibers::context::active()->get_properties();
        // Could still be hosed if the running manager isn't a subclass of
        // algorithm_with_properties.
        BOOST_ASSERT_MSG(props, "this_fiber::properties not set");
    }
    return dynamic_cast< PROPS & >( * props );
}

}

namespace fibers {

inline
bool has_ready_fibers() noexcept {
    return memoria::v1::fibers::context::active()->get_scheduler()->has_ready_fibers();
}

template< typename SchedAlgo, typename ... Args >
void use_scheduling_algorithm( Args && ... args) noexcept {
    memoria::v1::fibers::context::active()->get_scheduler()
        ->set_algo(
            std::unique_ptr< SchedAlgo >(
                new SchedAlgo( std::forward< Args >( args) ... ) ) );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif


