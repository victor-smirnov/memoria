
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <type_traits>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/detail/apply.hpp>
#include <boost/context/execution_context.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/parent_from_member.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/set.hpp>

#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/detail/data.hpp>
#include <memoria/v1/fiber/detail/decay_copy.hpp>
#include <memoria/v1/fiber/detail/fss.hpp>
#include <memoria/v1/fiber/detail/spinlock.hpp>
#include <memoria/v1/fiber/detail/wrap.hpp>
#include <memoria/v1/fiber/exceptions.hpp>
#include <memoria/v1/fiber/fixedsize_stack.hpp>
#include <memoria/v1/fiber/properties.hpp>
#include <memoria/v1/fiber/segmented_stack.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace v1 {    
namespace fibers {

enum class type {
    none               = 0,
    main_context       = 1 << 1,
    dispatcher_context = 1 << 2,
    worker_context     = 1 << 3,
    pinned_context     = main_context | dispatcher_context
};

inline
constexpr type
operator&( type l, type r) {
    return static_cast< type >(
            static_cast< unsigned int >( l) & static_cast< unsigned int >( r) );
}

inline
constexpr type
operator|( type l, type r) {
    return static_cast< type >(
            static_cast< unsigned int >( l) | static_cast< unsigned int >( r) );
}

inline
constexpr type
operator^( type l, type r) {
    return static_cast< type >(
            static_cast< unsigned int >( l) ^ static_cast< unsigned int >( r) );
}

inline
constexpr type
operator~( type l) {
    return static_cast< type >( ~static_cast< unsigned int >( l) );
}

inline
type &
operator&=( type & l, type r) {
    l = l & r;
    return l;
}

inline
type &
operator|=( type & l, type r) {
    l = l | r;
    return l;
}

inline
type &
operator^=( type & l, type r) {
    l = l ^ r;
    return l;
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif


