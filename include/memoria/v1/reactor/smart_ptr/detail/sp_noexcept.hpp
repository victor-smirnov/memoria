#ifndef MMA1_SMART_PTR_DETAIL_SP_NOEXCEPT_HPP_INCLUDED
#define MMA1_SMART_PTR_DETAIL_SP_NOEXCEPT_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/sp_noexcept.hpp
//
//  Copyright 2016, 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>

// noexcept

#if defined( BOOST_MSVC ) && BOOST_MSVC >= 1700 && BOOST_MSVC < 1900

#  define noexcept noexcept

#else

#  define noexcept noexcept

#endif

// noexcept_WITH_ASSERT

#if defined(BOOST_DISABLE_ASSERTS) || ( defined(BOOST_ENABLE_ASSERT_DEBUG_HANDLER) && defined(NDEBUG) )

#  define noexcept_WITH_ASSERT noexcept

#elif defined(BOOST_ENABLE_ASSERT_HANDLER) || ( defined(BOOST_ENABLE_ASSERT_DEBUG_HANDLER) && !defined(NDEBUG) )

#  define noexcept_WITH_ASSERT

#else

#  define noexcept_WITH_ASSERT noexcept

#endif

#endif  // #ifndef MMA1_SMART_PTR_DETAIL_SP_NOEXCEPT_HPP_INCLUDED
