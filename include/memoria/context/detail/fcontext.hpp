
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_DETAIL_FCONTEXT_H
#define MEMORIA_CONTEXT_DETAIL_FCONTEXT_H

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include <memoria/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace context {
namespace detail {

typedef void*   fcontext_t;

struct transfer_t {
    fcontext_t  fctx;
    void    *   data;
};

extern "C" MEMORIA_CONTEXT_DECL
transfer_t MEMORIA_CONTEXT_CALLDECL jump_fcontext( fcontext_t const to, void * vp);
extern "C" MEMORIA_CONTEXT_DECL
fcontext_t MEMORIA_CONTEXT_CALLDECL make_fcontext( void * sp, std::size_t size, void (* fn)( transfer_t) );

// based on an idea of Giovanni Derreta
extern "C" MEMORIA_CONTEXT_DECL
transfer_t MEMORIA_CONTEXT_CALLDECL ontop_fcontext( fcontext_t const to, void * vp, transfer_t (* fn)( transfer_t) );

}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_CONTEXT_DETAIL_FCONTEXT_H

