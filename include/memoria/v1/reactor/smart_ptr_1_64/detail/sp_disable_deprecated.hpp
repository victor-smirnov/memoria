#pragma once
//
//  memoria/v1/reactor/smart_ptr/detail/sp_disable_deprecated.hpp
//
//  Copyright 2015 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>

#if defined( __GNUC__ ) && ( defined( __GXX_EXPERIMENTAL_CXX0X__ ) || ( __cplusplus >= 201103L ) )

# if defined( BOOST_GCC )

#  if BOOST_GCC >= 40600
#   define MMA1_SP_DISABLE_DEPRECATED
#  endif

# elif defined( __clang__ ) && defined( __has_warning )

#  if __has_warning( "-Wdeprecated-declarations" )
#   define MMA1_SP_DISABLE_DEPRECATED
#  endif

# endif

#endif

