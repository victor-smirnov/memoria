#pragma once

//  detail/sp_counted_base_std_atomic.hpp - C++11 std::atomic
//
//  Copyright (c) 2007, 2013 Peter Dimov
//  Copyright 2017 Victor Smirnov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/detail/sp_typeinfo.hpp>
#include <atomic>
#include <cstdint>

namespace memoria {
namespace v1 {
namespace reactor {


namespace _
{

inline void atomic_increment( std::atomic_int_least32_t * pw )
{
    pw->fetch_add( 1, std::memory_order_relaxed );
}

inline void nonatomic_increment(std::int_least32_t * pw )
{
    (void)(*pw)++;
}

inline std::int_least32_t nonatomic_decrement( std::int_least32_t * pw )
{
    return (*pw)--;
}

inline std::int_least32_t atomic_decrement( std::atomic_int_least32_t * pw )
{
    return pw->fetch_sub( 1, std::memory_order_acq_rel );
}


inline std::int_least32_t atomic_conditional_increment( std::atomic_int_least32_t * pw )
{
//     long r = *pw;
//     if( r != 0 ) ++*pw;
//     return r;

    std::int_least32_t r = pw->load( std::memory_order_relaxed );

    for( ;; )
    {
        if( r == 0 )
        {
            return r;
        }

        if( pw->compare_exchange_weak( r, r + 1, std::memory_order_relaxed, std::memory_order_relaxed ) )
        {
            return r;
        }
    }
}

inline std::int_least32_t nonatomic_conditional_increment( std::int_least32_t * pw )
{
    std::int_least32_t r = *pw;
    if( r != 0 ) ++(*pw);
    return r;
}



class sp_counted_base;


class sp_reactor_counted_base {

    std::atomic_int_least32_t use_count_;	// #shared
    std::atomic_int_least32_t weak_count_;	// #weak + (#shared != 0)

    int cpu_;

public:

    sp_reactor_counted_base( sp_reactor_counted_base const & ) = delete;
    sp_reactor_counted_base & operator= ( sp_reactor_counted_base const & ) = delete;

    sp_reactor_counted_base(int cpu): use_count_( 1 ), weak_count_( 1 ), cpu_(cpu)
    {
    }

    virtual ~sp_reactor_counted_base() // nothrow
    {
    }

    int cpu() const {return cpu_;}

    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

    virtual void dispose() = 0; // nothrow

    // destroy() is called when weak_count_ drops to zero.

    virtual void destroy() noexcept
    {
        free(this);
    }

    virtual void * get_deleter( boost::detail::sp_typeinfo const & ti ) = 0;
    virtual void * get_untyped_deleter() = 0;

    void add_ref_copy()
    {
        atomic_increment( &use_count_ );
    }

    bool add_ref_lock() // true on success
    {
        return atomic_conditional_increment( &use_count_ ) != 0;
    }

    void release() // nothrow
    {
        if( atomic_decrement( &use_count_ ) == 1 )
        {
            dispose();
            weak_release();
        }
    }

    void weak_add_ref() // nothrow
    {
        atomic_increment( &weak_count_ );
    }

    void weak_release() // nothrow
    {
        if( atomic_decrement( &weak_count_ ) == 1 )
        {
            destroy();
        }
    }

    long use_count() const // nothrow
    {
        return use_count_.load( std::memory_order_acquire );
    }

    virtual void set_local_counter(int cpu, sp_counted_base* counter) = 0;
    virtual sp_counted_base* get_local_counter(int cpu) = 0;
};





class sp_counted_base
{
    std::int_least32_t use_count_;      // #shared
    std::int_least32_t weak_count_;     // #weak + (#shared != 0)

public:
    sp_counted_base( sp_counted_base const & ) = delete;
    sp_counted_base & operator= ( sp_counted_base const & ) = delete;


    sp_counted_base(): use_count_( 1 ), weak_count_( 1 )
    {
    }

    virtual ~sp_counted_base() // nothrow
    {
    }

    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

    virtual void dispose() = 0; // nothrow

    // destroy() is called when weak_count_ drops to zero.

    virtual void destroy() noexcept
    {
        delete this;
    }

    virtual void * get_deleter( boost::detail::sp_typeinfo const & ti ) = 0;
    virtual void * get_untyped_deleter() = 0;

    void add_ref_copy()
    {
        nonatomic_increment( &use_count_ );
    }

    bool add_ref_lock() noexcept // true on success
    {
        return nonatomic_conditional_increment( &use_count_ ) != 0;
    }

    void release() noexcept
    {
        if( nonatomic_decrement( &use_count_ ) == 1 )
        {
            dispose();
            weak_release();
        }
    }

    void weak_add_ref() noexcept
    {
        nonatomic_increment( &weak_count_ );
    }

    void weak_release() noexcept
    {
        if( nonatomic_decrement( &weak_count_ ) == 1 )
        {
            destroy();
        }
    }

    long use_count() const noexcept
    {
        return use_count_;
    }
};







} // namespace detail

}}}
