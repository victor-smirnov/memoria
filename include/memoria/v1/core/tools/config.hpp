
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#ifdef _INTEL_COMPILER

#pragma warning (disable : 2586)
#pragma warning (disable : 1125)
#pragma warning (disable : 869)

#elif defined(_MSC_VER)

#pragma warning (disable : 4503)
#pragma warning (disable : 4355)
#pragma warning (disable : 4244)
#pragma warning (disable : 4231)
#pragma warning (disable : 4800)

#elif defined(__clang__)

#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wunused-private-field"

#endif




#ifndef MEMORIA_AT

#define MEMORIA_JOIN( X, Y ) MEMORIA_INT0_JOIN( X, Y )
#define MEMORIA_INT0_JOIN( X, Y ) MEMORIA_INT1_JOIN(X,Y)
#define MEMORIA_INT1_JOIN( X, Y ) X##Y

#define MEMORIA_STRINGIFY(x) #x
#define MEMORIA_TOSTRING(x) MEMORIA_STRINGIFY(x)

#define MEMORIA_AT MEMORIA_TOSTRING(__LINE__)

#endif





#ifndef MEMORIA_SOURCE

#define MEMORIA_SOURCE (__FILE__ ":" MEMORIA_AT)
#define MEMORIA_RAW_SOURCE (__FILE__ ":" MEMORIA_AT)
#define MA_SRC MEMORIA_SOURCE
#define MA_RAW_SRC MEMORIA_RAW_SOURCE

#endif
