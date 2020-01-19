
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_DETAIL_EXCEPTION_H
#define MEMORIA_CONTEXT_DETAIL_EXCEPTION_H

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <memoria/v1/context/detail/fcontext.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace context {
namespace detail {

struct forced_unwind {
    fcontext_t  fctx{ nullptr };
#ifndef BOOST_ASSERT_IS_VOID
    bool        caught{ false };
#endif

    forced_unwind() = default;

    forced_unwind( fcontext_t fctx_) :
        fctx( fctx_) {
    }

#ifndef BOOST_ASSERT_IS_VOID
    ~forced_unwind() {
        BOOST_ASSERT( caught);
    }
#endif
};

}}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_CONTEXT_DETAIL_EXCEPTION_H