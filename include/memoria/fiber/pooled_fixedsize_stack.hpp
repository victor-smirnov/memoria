
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_POOLED_FIXEDSIZE_STACK_H
#define MEMORIA_FIBERS_POOLED_FIXEDSIZE_STACK_H

#include <boost/config.hpp>
#include <memoria/context/pooled_fixedsize_stack.hpp>

#include <memoria/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace fibers {

using pooled_fixedsize_stack = memoria::context::pooled_fixedsize_stack;

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_POOLED_FIXEDSIZE_STACK_H
