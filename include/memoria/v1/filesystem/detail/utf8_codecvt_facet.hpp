// Copyright (c) 2001 Ronald Garcia, Indiana University (garcia@osl.iu.edu)
// Andrew Lumsdaine, Indiana University (lums@osl.iu.edu).

// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../config.hpp"

#define BOOST_UTF8_BEGIN_NAMESPACE \
     namespace memoria { namespace v1 { namespace filesystem { namespace detail {

#define BOOST_UTF8_END_NAMESPACE }}}}
#define BOOST_UTF8_DECL MEMORIA_V1_FILESYSTEM_DECL

#include <boost/detail/utf8_codecvt_facet.hpp>

#undef BOOST_UTF8_BEGIN_NAMESPACE
#undef BOOST_UTF8_END_NAMESPACE
#undef BOOST_UTF8_DECL


