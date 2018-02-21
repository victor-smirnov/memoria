
//
//  memoria/v1/reactor/smart_ptr/detail/sp_has_sync.hpp
//
//  Copyright (c) 2008, 2009 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  Defines the MMA1_SP_HAS_SYNC macro if the __sync_* intrinsics
//  are available.
//

#pragma once

#ifndef MMA1_SP_NO_SYNC

#if !defined( __c2__ ) && defined( __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 )

# define MMA1_SP_HAS_SYNC

#elif defined( __IBMCPP__ ) && ( __IBMCPP__ >= 1210 ) && !defined( __COMPILER_VER__ )

# define MMA1_SP_HAS_SYNC

#elif !defined( __c2__ ) && defined( __GNUC__ ) && ( __GNUC__ * 100 + __GNUC_MINOR__ >= 401 )

#define MMA1_SP_HAS_SYNC

#if defined( __arm__ )  || defined( __armel__ )
#undef MMA1_SP_HAS_SYNC
#endif

#if defined( __hppa ) || defined( __hppa__ )
#undef MMA1_SP_HAS_SYNC
#endif

#if defined( __m68k__ )
#undef MMA1_SP_HAS_SYNC
#endif

#if defined( __sh__ )
#undef MMA1_SP_HAS_SYNC
#endif

#if defined( __sparc__ )
#undef MMA1_SP_HAS_SYNC
#endif

#if defined( __INTEL_COMPILER ) && !defined( __ia64__ ) && ( __INTEL_COMPILER < 1110 )
#undef MMA1_SP_HAS_SYNC
#endif

#if defined(__PATHSCALE__) && ((__PATHCC__ == 4) && (__PATHCC_MINOR__ < 9)) 
#undef MMA1_SP_HAS_SYNC
#endif

#endif

#endif // #ifndef MMA1_SP_NO_SYNC
