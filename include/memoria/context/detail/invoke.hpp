
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_DETAIL_INVOKE_H
#define MEMORIA_CONTEXT_DETAIL_INVOKE_H

#include <functional>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>
#include <boost/utility/result_of.hpp>

#include <memoria/context/detail/config.hpp>


namespace memoria {
namespace context {
namespace detail {

template< typename Fn, typename ... Args >
typename std::enable_if<
    std::is_member_pointer< typename std::decay< Fn >::type >::value,
    typename boost::result_of< Fn &&( Args && ... ) >::type
>::type
invoke( Fn && fn, Args && ... args) {
    return std::mem_fn( fn)( std::forward< Args >( args) ... );   
}

template< typename Fn, typename ... Args >
typename std::enable_if<
    ! std::is_member_pointer< typename std::decay< Fn >::type >::value,
    typename boost::result_of< Fn &&( Args && ... ) >::type
>::type
invoke( Fn && fn, Args && ... args) {
    return std::forward< Fn >( fn)( std::forward< Args >( args) ... );
}

}}}


#endif // MEMORIA_CONTEXT_DETAIL_INVOKE_H
