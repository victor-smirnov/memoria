
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_STACK_CONTEXT_H
#define MEMORIA_CONTEXT_STACK_CONTEXT_H

#include <cstddef>

#include <boost/config.hpp>

#include <memoria/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace context {


struct MEMORIA_CONTEXT_DECL stack_context {
# if defined(MEMORIA_USE_SEGMENTED_STACKS)
    typedef void *  segments_context[MEMORIA_CONTEXT_SEGMENTS];
# endif

    std::size_t             size{ 0 };
    void                *   sp{ nullptr };
# if defined(MEMORIA_USE_SEGMENTED_STACKS)
    segments_context        segments_ctx{};
# endif
# if defined(BOOST_USE_VALGRIND)
    unsigned                valgrind_stack_id{ 0 };
# endif
#if defined(MEMORIA_USE_ASAN)
    void*           fake_stack{ nullptr };
    const void*     stack_bottom{ nullptr };
    std::size_t     stack_size{ 0 };
#endif
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_CONTEXT_STACK_CONTEXT_H
