
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_PREALLOCATED_H
#define MEMORIA_CONTEXT_PREALLOCATED_H

#include <cstddef>

#include <boost/config.hpp>

#include <memoria/v1/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace context {

struct preallocated {
    void        *   sp;
    std::size_t     size;
    stack_context   sctx;

    preallocated( void * sp_, std::size_t size_, stack_context sctx_) noexcept :
        sp( sp_), size( size_), sctx( sctx_) {
    }
};

}}}
#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_CONTEXT_PREALLOCATED_H
