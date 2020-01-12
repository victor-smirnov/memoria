
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_SEGMENTED_STACK_H
#define MEMORIA_FIBERS_SEGMENTED_STACK_H

#include <boost/config.hpp>
#include <memoria/v1/context/segmented_stack.hpp>

#include <memoria/v1/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {

#if defined(MEMORIA_USE_SEGMENTED_STACKS)
# if ! defined(BOOST_WINDOWS)
using segmented_stack = memoria::v1::context::segmented_stack;
using default_stack = memoria::v1::context::default_stack;
# endif
#endif

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_SEGMENTED_STACK_H
