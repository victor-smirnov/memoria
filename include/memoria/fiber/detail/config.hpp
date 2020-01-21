
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_DETAIL_CONFIG_H
#define MEMORIA_FIBERS_DETAIL_CONFIG_H

#include <cstddef>


#include <memoria/core/config.hpp>

#include <boost/config.hpp>
#include <boost/predef.h> 
#include <boost/detail/workaround.hpp>

#ifdef MEMORIA_FIBERS_DECL
# undef MEMORIA_FIBERS_DECL
#endif

#if (defined(BOOST_ALL_DYN_LINK) || defined(MEMORIA_FIBERS_DYN_LINK) ) && ! defined(MEMORIA_FIBERS_STATIC_LINK)
# if defined(MEMORIA_FIBERS_SOURCE)
#  define MEMORIA_FIBERS_DECL MEMORIA_API
#  define MEMORIA_FIBERS_BUILD_DLL
# else
#  define MEMORIA_FIBERS_DECL MEMORIA_API
# endif
#endif

#if ! defined(MEMORIA_FIBERS_DECL)
# define MEMORIA_FIBERS_DECL
#endif

#if ! defined(MEMORIA_FIBERS_SOURCE) && ! defined(BOOST_ALL_NO_LIB) && ! defined(MEMORIA_FIBERS_NO_LIB)
# define BOOST_LIB_NAME boost_fiber
# if defined(BOOST_ALL_DYN_LINK) || defined(MEMORIA_FIBERS_DYN_LINK)
#  define BOOST_DYN_LINK
# endif
# include <boost/config/auto_link.hpp>
#endif

#if BOOST_OS_LINUX || BOOST_OS_WINDOWS
# define MEMORIA_FIBERS_HAS_FUTEX
#endif

#if (!defined(MEMORIA_FIBERS_HAS_FUTEX) && \
    (defined(MEMORIA_FIBERS_SPINLOCK_TTAS_FUTEX) || defined(MEMORIA_FIBERS_SPINLOCK_TTAS_ADAPTIVE_FUTEX)))
# error "futex not supported on this platform"
#endif

#if !defined(MEMORIA_FIBERS_CONTENTION_WINDOW_THRESHOLD)
# define MEMORIA_FIBERS_CONTENTION_WINDOW_THRESHOLD 16
#endif

#if !defined(MEMORIA_FIBERS_RETRY_THRESHOLD)
# define MEMORIA_FIBERS_RETRY_THRESHOLD 64
#endif

#if !defined(MEMORIA_FIBERS_SPIN_BEFORE_SLEEP0)
# define MEMORIA_FIBERS_SPIN_BEFORE_SLEEP0 32
#endif

#if !defined(MEMORIA_FIBERS_SPIN_BEFORE_YIELD)
# define MEMORIA_FIBERS_SPIN_BEFORE_YIELD 64
#endif

#endif // MEMORIA_FIBERS_DETAIL_CONFIG_H
