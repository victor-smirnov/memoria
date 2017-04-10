// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// For HP-UX, request that WCHAR_MAX and WCHAR_MIN be defined as macros,
// not casts. See ticket 5048
#define _INCLUDE_STDCSOURCE_199901

#ifndef BOOST_SYSTEM_NO_DEPRECATED 
# define BOOST_SYSTEM_NO_DEPRECATED
#endif

#define MEMORIA_V1_FILESYSTEM_SOURCE
#include <memoria/v1/filesystem/config.hpp>

#define BOOST_UTF8_BEGIN_NAMESPACE \
     namespace memoria { namespace v1 { namespace filesystem { namespace detail {

#define BOOST_UTF8_END_NAMESPACE }}}}
#define BOOST_UTF8_DECL MEMORIA_V1_FILESYSTEM_DECL

#include <boost/detail/utf8_codecvt_facet.ipp>

#undef BOOST_UTF8_BEGIN_NAMESPACE
#undef BOOST_UTF8_END_NAMESPACE
#undef BOOST_UTF8_DECL
