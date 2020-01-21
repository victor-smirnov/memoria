//  boost/filesystem/v3/config.hpp  ----------------------------------------------------//

//  Copyright Beman Dawes 2003

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#include <memoria/core/config.hpp>

#ifndef MEMORIA_BOOST_FILESYSTEM3_CONFIG_HPP
#define MEMORIA_BOOST_FILESYSTEM3_CONFIG_HPP

# if defined(MEMORIA_BOOST_FILESYSTEM_VERSION) && MEMORIA_BOOST_FILESYSTEM_VERSION != 3
#   error Compiling Filesystem version 3 file with MEMORIA_BOOST_FILESYSTEM_VERSION defined != 3
# endif

# if !defined(MEMORIA_BOOST_FILESYSTEM_VERSION)
#   define MEMORIA_BOOST_FILESYSTEM_VERSION 3
# endif

#define MEMORIA_BOOST_FILESYSTEM_I18N  // aid users wishing to compile several versions

// This header implements separate compilation features as described in
// http://www.boost.org/more/separate_compilation.html

#include <boost/config.hpp>
#include <boost/system/api_config.hpp>  // for BOOST_POSIX_API or BOOST_WINDOWS_API
#include <boost/detail/workaround.hpp>

//  MEMORIA_BOOST_FILESYSTEM_DEPRECATED needed for source compiles -----------------------------//

# ifdef MEMORIA_BOOST_FILESYSTEM_SOURCE
#   define MEMORIA_BOOST_FILESYSTEM_DEPRECATED
#   undef MEMORIA_BOOST_FILESYSTEM_NO_DEPRECATED   // fixes #9454, src bld fails if NO_DEP defined
# endif

//  throw an exception  ----------------------------------------------------------------//
//
//  Exceptions were originally thrown via boost::throw_exception().
//  As throw_exception() became more complex, it caused user error reporting
//  to be harder to interpret, since the exception reported became much more complex.
//  The immediate fix was to throw directly, wrapped in a macro to make any later change
//  easier.

#define MEMORIA_BOOST_FILESYSTEM_THROW(EX) throw EX

# if defined( BOOST_NO_STD_WSTRING )
#   error Configuration not supported: Boost.Filesystem V3 and later requires std::wstring support
# endif

//  This header implements separate compilation features as described in
//  http://www.boost.org/more/separate_compilation.html

//  normalize macros  ------------------------------------------------------------------//

#if !defined(MEMORIA_BOOST_FILESYSTEM_DYN_LINK) && !defined(MEMORIA_BOOST_FILESYSTEM_STATIC_LINK) \
  && !defined(BOOST_ALL_DYN_LINK) && !defined(BOOST_ALL_STATIC_LINK)
# define MEMORIA_BOOST_FILESYSTEM_STATIC_LINK
#endif

#if defined(BOOST_ALL_DYN_LINK) && !defined(MEMORIA_BOOST_FILESYSTEM_DYN_LINK)
# define MEMORIA_BOOST_FILESYSTEM_DYN_LINK
#elif defined(BOOST_ALL_STATIC_LINK) && !defined(MEMORIA_BOOST_FILESYSTEM_STATIC_LINK)
# define MEMORIA_BOOST_FILESYSTEM_STATIC_LINK
#endif

#if defined(MEMORIA_BOOST_FILESYSTEM_DYN_LINK) && defined(MEMORIA_BOOST_FILESYSTEM_STATIC_LINK)
# error Must not define both MEMORIA_BOOST_FILESYSTEM_DYN_LINK and MEMORIA_BOOST_FILESYSTEM_STATIC_LINK
#endif

#if defined(BOOST_ALL_NO_LIB) && !defined(MEMORIA_BOOST_FILESYSTEM_NO_LIB)
# define MEMORIA_BOOST_FILESYSTEM_NO_LIB
#endif

//  enable dynamic linking  ------------------------------------------------------------//

#if defined(BOOST_ALL_DYN_LINK) || defined(MEMORIA_BOOST_FILESYSTEM_DYN_LINK)
# if defined(MEMORIA_BOOST_FILESYSTEM_SOURCE)
#   define MEMORIA_BOOST_FILESYSTEM_DECL MEMORIA_API
# else
#   define MEMORIA_BOOST_FILESYSTEM_DECL MEMORIA_API
# endif
#else
# define MEMORIA_BOOST_FILESYSTEM_DECL
#endif

//  enable automatic library variant selection  ----------------------------------------//

#if !defined(MEMORIA_BOOST_FILESYSTEM_SOURCE) && !defined(BOOST_ALL_NO_LIB) \
  && !defined(MEMORIA_BOOST_FILESYSTEM_NO_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define BOOST_LIB_NAME boost_filesystem
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#if defined(BOOST_ALL_DYN_LINK) || defined(MEMORIA_BOOST_FILESYSTEM_DYN_LINK)
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#endif // MEMORIA_BOOST_FILESYSTEM3_CONFIG_HPP
