
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>
#include <memoria/context/detail/config.hpp>

#if defined(MEMORIA_USE_ASAN)
extern "C" {
void __sanitizer_start_switch_fiber( void **, const void *, size_t);
void __sanitizer_finish_switch_fiber( void *, const void **, size_t *);
}
#endif

#if defined(MEMORIA_USE_SEGMENTED_STACKS)
extern "C" {
void __splitstack_getcontext( void * [MEMORIA_CONTEXT_SEGMENTS]);
void __splitstack_setcontext( void * [MEMORIA_CONTEXT_SEGMENTS]);
}
#endif
