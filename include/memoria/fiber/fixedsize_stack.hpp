
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_FIXEDSIZE_STACK_H
#define MEMORIA_FIBERS_FIXEDSIZE_STACK_H

#include <boost/config.hpp>
#include <memoria/context/fixedsize_stack.hpp>

#include <memoria/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace fibers {

using fixedsize_stack = memoria::context::fixedsize_stack;
#if !defined(MEMORIA_USE_SEGMENTED_STACKS)
using   default_stack = memoria::context::default_stack;
#endif

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_FIXEDSIZE_STACK_H
