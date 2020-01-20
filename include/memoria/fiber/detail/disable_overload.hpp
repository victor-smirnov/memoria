
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBER_DETAIL_DISABLE_OVERLOAD_H
#define BOOST_FIBER_DETAIL_DISABLE_OVERLOAD_H

#include <type_traits>

#include <boost/config.hpp>
#include <memoria/context/detail/disable_overload.hpp>

#include <memoria/fiber/detail/config.hpp>


namespace memoria {
namespace fibers {
namespace detail {

template< typename X, typename Y >
using disable_overload = memoria::context::detail::disable_overload< X, Y >;

}}}


#endif // BOOST_FIBER_DETAIL_DISABLE_OVERLOAD_H
