
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>

#include <memoria/v1/fiber/future/future.hpp>
#include <memoria/v1/fiber/future/packaged_task.hpp>
#include <memoria/v1/fiber/policy.hpp>

namespace memoria {
namespace v1 {    
namespace fibers {

template< typename Fn, typename ... Args >
future<
	typename std::result_of<
		typename std::enable_if<
			! detail::is_launch_policy< typename std::decay< Fn >::type >::value,
			typename std::decay< Fn >::type
		>::type( typename std::decay< Args >::type ... )
	>::type
>
async( Fn && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::forward< Fn >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< typename Policy, typename Fn, typename ... Args >
future<
	typename std::result_of<
		typename std::enable_if<
			detail::is_launch_policy< Policy >::value,
			typename std::decay< Fn >::type
		>::type( typename std::decay< Args >::type ...)
	>::type
>
async( Policy policy, Fn && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::forward< Fn >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ policy, std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< typename Policy, typename StackAllocator, typename Fn, typename ... Args >
future<
	typename std::result_of<
		typename std::enable_if<
			detail::is_launch_policy< Policy >::value,
			typename std::decay< Fn >::type
		>::type( typename std::decay< Args >::type ... )
	>::type
>
async( Policy policy, std::allocator_arg_t, StackAllocator salloc, Fn && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::allocator_arg, salloc, std::forward< Fn >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ policy, std::allocator_arg, salloc,
        std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

}}}


