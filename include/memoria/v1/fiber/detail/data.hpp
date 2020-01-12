
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_DETAIL_DATA_H
#define MEMORIA_FIBERS_DETAIL_DATA_H

#include <boost/config.hpp>

#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {

class context;

namespace detail {

struct data_t {
    spinlock_lock   *   lk{ nullptr };
    context         *   ctx{ nullptr };
    context         *   from;

    explicit data_t( context * from_) noexcept :
        from{ from_ } {
    }

    explicit data_t( spinlock_lock * lk_,
                     context * from_) noexcept :
        lk{ lk_ },
        from{ from_ } {
    }

    explicit data_t( context * ctx_,
                     context * from_) noexcept :
        ctx{ ctx_ },
        from{ from_ } {
    }
};

}}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_DETAIL_DATA_H
