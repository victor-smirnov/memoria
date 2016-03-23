
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifdef PAGE_SIZE
#define SYSTEM_PAGESIZE PAGE_SIZE
#undef PAGE_SIZE
#endif


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





#ifndef MEMORIA_STATIC
#if defined (WIN32)
#   define MEMORIA_EXPORT __declspec(dllexport)
#   define MEMORIA_NO_EXPORT
#   define MEMORIA_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#       define MEMORIA_EXPORT __attribute__((visibility("default")))
#       define MEMORIA_NO_EXPORT __attribute__((visibility("hidden")))
#       define MEMORIA_IMPORT
#else
#       define MEMORIA_EXPORT
#       define MEMORIA_NO_EXPORT
#       define MEMORIA_IMPORT
#endif
#else
#       define MEMORIA_EXPORT
#       define MEMORIA_NO_EXPORT
#       define MEMORIA_IMPORT
#endif

#if defined(MEMORIA_MAIN)
#define MEMORIA_API MEMORIA_EXPORT
#else
#define MEMORIA_API MEMORIA_IMPORT
#endif


#define MEMORIA_JOIN( X, Y ) MEMORIA_INT0_JOIN( X, Y )
#define MEMORIA_INT0_JOIN( X, Y ) MEMORIA_INT1_JOIN(X,Y)
#define MEMORIA_INT1_JOIN( X, Y ) X##Y

#define MEMORIA_STRINGIFY(x) #x
#define MEMORIA_TOSTRING(x) MEMORIA_STRINGIFY(x)

#define MEMORIA_AT MEMORIA_TOSTRING(__LINE__)

#define MEMORIA_SOURCE ::memoria::ExtractMemoriaPath(__FILE__ ":" MEMORIA_AT)
#define MEMORIA_RAW_SOURCE ::memoria::ExtractMemoriaPath(__FILE__ ":" MEMORIA_AT)
#define MA_SRC MEMORIA_SOURCE
#define MA_RAW_SRC MEMORIA_RAW_SOURCE

#define MEMORIA_PUBLIC
#define MEMORIA_DEPRECATED

#if !defined(MEMORIA_DLL) && !defined(MEMORIA_MAIN)
#define MEMORIA_TEMPLATE_EXTERN extern
#else
#define MEMORIA_TEMPLATE_EXTERN
#endif
