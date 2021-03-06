
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_SEGMENTED_H
#define MEMORIA_CONTEXT_SEGMENTED_H

#include <cstddef>
#include <new>

#include <boost/config.hpp>

#include <memoria/context/detail/config.hpp>
#include <memoria/context/stack_context.hpp>
#include <memoria/context/stack_traits.hpp>


// forward declaration for splitstack-functions defined in libgcc
extern "C" {
void *__splitstack_makecontext( std::size_t,
                                void * [MEMORIA_CONTEXT_SEGMENTS],
                                std::size_t *);

void __splitstack_releasecontext( void * [MEMORIA_CONTEXT_SEGMENTS]);

void __splitstack_resetcontext( void * [MEMORIA_CONTEXT_SEGMENTS]);

void __splitstack_block_signals_context( void * [MEMORIA_CONTEXT_SEGMENTS],
                                         int * new_value, int * old_value);
}

namespace memoria {
namespace context {

template< typename traitsT >
class basic_segmented_stack {
private:
    std::size_t     size_;

public:
    typedef traitsT traits_type;

    basic_segmented_stack( std::size_t size = traits_type::default_size() ) noexcept :
        size_( size) {
    }

    stack_context allocate() {
        stack_context sctx;
        void * vp = __splitstack_makecontext( size_, sctx.segments_ctx, & sctx.size);
        if ( ! vp) throw std::bad_alloc();

        // sctx.size is already filled by __splitstack_makecontext
        sctx.sp = static_cast< char * >( vp) + sctx.size;

        int off = 0;
        __splitstack_block_signals_context( sctx.segments_ctx, & off, 0);

        return sctx;
    }

    void deallocate( stack_context & sctx) noexcept {
        __splitstack_releasecontext( sctx.segments_ctx);
    }
};

typedef basic_segmented_stack< stack_traits > segmented_stack;
# if defined(MEMORIA_USE_SEGMENTED_STACKS)
typedef segmented_stack default_stack;
# endif

}}


#endif // MEMORIA_CONTEXT_SEGMENTED_H
