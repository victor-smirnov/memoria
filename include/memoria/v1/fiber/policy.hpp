
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_POLICY_H
#define MEMORIA_FIBERS_POLICY_H

#include <type_traits>

#include <boost/config.hpp>

#include <memoria/v1/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {

enum class launch {
    dispatch,
    post
};

namespace detail {

template< typename Fn >
struct is_launch_policy : public std::false_type {
};

template<>
struct is_launch_policy< memoria::v1::fibers::launch > : public std::true_type {
};

}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_POLICY_H
