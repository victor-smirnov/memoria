
//
//  boost/detail/atomic_count.hpp
//
//  atomic_count for std::atomic
//
//  Copyright 2013 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#pragma once

#include <atomic>
#include <cstdint>

namespace memoria {
namespace v1 {
namespace reactor {

namespace detail
{

class atomic_count
{
public:

    explicit atomic_count( long v ): value_( v )
    {
    }

    long operator++()
    {
        return value_.fetch_add( 1, std::memory_order_acq_rel ) + 1;
    }

    long operator--()
    {
        return value_.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
    }

    operator long() const
    {
        return value_.load( std::memory_order_acquire );
    }

private:

    atomic_count(atomic_count const &);
    atomic_count & operator=(atomic_count const &);

    std::atomic_int_least32_t value_;
};

} // namespace detail

}}}


