
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/context/stack_traits.hpp>

extern "C" {
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
}

//#if _POSIX_C_SOURCE >= 200112L

#include <algorithm>
#include <cmath>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#if defined(BOOST_NO_CXX11_HDR_MUTEX)
# include <boost/thread.hpp>
#else
# include <mutex>
#endif

#if !defined (SIGSTKSZ)
# define SIGSTKSZ (32768) // 32kb minimum allowable stack
# define UDEF_SIGSTKSZ
#endif

#if !defined (MINSIGSTKSZ)
# define MINSIGSTKSZ (131072) // 128kb recommended stack size
# define UDEF_MINSIGSTKSZ
#endif


namespace {

void pagesize_( std::size_t * size) noexcept {
    // conform to POSIX.1-2001
    * size = ::sysconf( _SC_PAGESIZE);
}

void stacksize_limit_( rlimit * limit) noexcept {
    // conforming to POSIX.1-2001
    ::getrlimit( RLIMIT_STACK, limit);
}

std::size_t pagesize() noexcept {
    static std::size_t size = 0;
#if defined(BOOST_NO_CXX11_HDR_MUTEX)
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once( flag, pagesize_, & size);
#else
    static std::once_flag flag;
    std::call_once( flag, pagesize_, & size);
#endif
    return size;
}

rlimit stacksize_limit() noexcept {
    static rlimit limit;
#if defined(BOOST_NO_CXX11_HDR_MUTEX)
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once( flag, stacksize_limit_, & limit);
#else
    static std::once_flag flag;
    std::call_once( flag, stacksize_limit_, & limit);
#endif
    return limit;
}

}

namespace memoria {
namespace context {

bool
stack_traits::is_unbounded() noexcept {
    return RLIM_INFINITY == stacksize_limit().rlim_max;
}

std::size_t
stack_traits::page_size() noexcept {
    return pagesize();
}

std::size_t
stack_traits::default_size() noexcept {
    return 128 * 1024;
}

std::size_t
stack_traits::minimum_size() noexcept {
    return MINSIGSTKSZ;
}

std::size_t
stack_traits::maximum_size() noexcept {
    BOOST_ASSERT( ! is_unbounded() );
    return static_cast< std::size_t >( stacksize_limit().rlim_max);
}

}}


#ifdef UDEF_SIGSTKSZ
# undef SIGSTKSZ;
#endif

#ifdef UDEF_MINSIGSTKSZ
# undef MINSIGSTKSZ
#endif
