
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <boost/config.hpp>
#include <boost/predef.h> 
#include <boost/detail/workaround.hpp>

#ifdef MEMORIA_V1_FIBERS_DECL
# undef MEMORIA_V1_FIBERS_DECL
#endif

#if (defined(BOOST_ALL_DYN_LINK) || defined(MEMORIA_V1_FIBERS_DYN_LINK) ) && ! defined(MEMORIA_V1_FIBERS_STATIC_LINK)
# if defined(MEMORIA_V1_FIBERS_SOURCE)
#  define MEMORIA_V1_FIBERS_DECL BOOST_SYMBOL_EXPORT
#  define MEMORIA_V1_FIBERS_BUILD_DLL
# else
#  define MEMORIA_V1_FIBERS_DECL BOOST_SYMBOL_IMPORT
# endif
#endif

#if ! defined(MEMORIA_V1_FIBERS_DECL)
# define MEMORIA_V1_FIBERS_DECL
#endif

#if ! defined(MEMORIA_V1_FIBERS_SOURCE) && ! defined(BOOST_ALL_NO_LIB) && ! defined(MEMORIA_V1_FIBERS_NO_LIB)
# define BOOST_LIB_NAME boost_fiber
# if defined(BOOST_ALL_DYN_LINK) || defined(MEMORIA_V1_FIBERS_DYN_LINK)
#  define BOOST_DYN_LINK
# endif
# include <boost/config/auto_link.hpp>
#endif

#if BOOST_OS_LINUX || BOOST_OS_WINDOWS
# define MEMORIA_V1_FIBERS_HAS_FUTEX
#endif

#if (!defined(MEMORIA_V1_FIBERS_HAS_FUTEX) && \
    (defined(MEMORIA_V1_FIBERS_SPINLOCK_TTAS_FUTEX) || defined(MEMORIA_V1_FIBERS_SPINLOCK_TTAS_ADAPTIVE_FUTEX)))
# error "futex not supported on this platform"
#endif

#if !defined(MEMORIA_V1_FIBERS_SPIN_MAX_COLLISIONS)
# define MEMORIA_V1_FIBERS_SPIN_MAX_COLLISIONS 16
#endif

#if !defined(MEMORIA_V1_FIBERS_SPIN_MAX_TESTS)
# define MEMORIA_V1_FIBERS_SPIN_MAX_TESTS 100
#endif

namespace memoria {
namespace v1 {
namespace fiber {

// modern architectures have cachelines with 64byte length
// ARM Cortex-A15 32/64byte, Cortex-A9 16/32/64bytes
// MIPS 74K: 32byte, 4KEc: 16byte
// ist shoudl be safe to use 64byte for all
static constexpr std::size_t cache_alignment{ 64 };
static constexpr std::size_t cacheline_length{ 64 };


}}}