/*
 * config.hpp
 *
 *  Created on: 15.09.2011
 *      Author: Victor
 */

#ifndef _MEMORIA_CORE_TOOLS_CONFIG_HPP12_
#define _MEMORIA_CORE_TOOLS_CONFIG_HPP12_

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
#endif

#ifndef MEMORIA_STATIC
#if defined (WIN32)
#	define MEMORIA_EXPORT __declspec(dllexport)
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

#define MEMORIA_SOURCE ::memoria::vapi::ExtractMemoriaPath(__FILE__ ":" MEMORIA_AT)

#define MEMORIA_PUBLIC

#if !defined(MEMORIA_DLL) && !defined(MEMORIA_MAIN)
#define MEMORIA_TEMPLATE_EXTERN extern
#else
#define MEMORIA_TEMPLATE_EXTERN
#endif

#endif
