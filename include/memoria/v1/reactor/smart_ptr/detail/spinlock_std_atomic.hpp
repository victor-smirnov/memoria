
//
//  Copyright (c) 2014 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/smart_ptr/detail/yield_k.hpp>
#include <atomic>

namespace memoria {
namespace v1 {
namespace reactor {


namespace detail
{

class spinlock
{
public:

    std::atomic_flag v_;

public:

    bool try_lock()
    {
        return !v_.test_and_set( std::memory_order_acquire );
    }

    void lock()
    {
        for( unsigned k = 0; !try_lock(); ++k )
        {
            boost::detail::yield( k );
        }
    }

    void unlock()
    {
        v_ .clear( std::memory_order_release );
    }

public:

    class scoped_lock
    {
    private:

        spinlock & sp_;

        scoped_lock( scoped_lock const & );
        scoped_lock & operator=( scoped_lock const & );

    public:

        explicit scoped_lock( spinlock & sp ): sp_( sp )
        {
            sp.lock();
        }

        ~scoped_lock()
        {
            sp_.unlock();
        }
    };
};

}

}}}

#define MMA1_SP_DETAIL_SPINLOCK_INIT { ATOMIC_FLAG_INIT }
