#pragma once
//
//  detail/sp_counted_impl.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//  Copyright 2017 Victor Smirnov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>



#include <boost/checked_delete.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_counted_base_std_atomic.hpp>

#include <memory>           // std::allocator
#include <cstddef>          // std::size_t

namespace memoria {
namespace v1 {
namespace reactor {




namespace _ {

class sp_local_counter: public sp_local_counted_base {
    sp_reactor_counted_base* owner_;
    int cpu_;
public:
    sp_local_counter(int cpu, sp_reactor_counted_base* owner):
        owner_(owner),
        cpu_(cpu)
    {}

    sp_reactor_counted_base* owner() {return owner_;}

    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

    virtual void dispose() {
        if (owner_->release()) {
            owner_ = nullptr;
        }
    }

    // destroy() is called when weak_count_ drops to zero.

    virtual void destroy() noexcept
    {
        if (owner_)
        {
            owner_->set_local_counter(cpu_, nullptr);
            owner_->weak_release();
        }

        delete this;
    }

    virtual void * get_deleter( boost::detail::sp_typeinfo const & ti ) {
        return owner_ ? owner_->get_deleter(ti) : nullptr;
    }

    virtual void * get_untyped_deleter() {
        return owner_ ? owner_->get_untyped_deleter(): nullptr;
    }
};



inline void initialize(int size, sp_local_counted_base** proxies)
{
    for (int c = 0; c < size; c++) {
        proxies[c] = nullptr;
    }

    std::atomic_thread_fence(std::memory_order_acquire);
}


template<class X>
class sp_reactor_counted_impl_p: public sp_reactor_counted_base
{
    X * px_;
    sp_local_counted_base* proxies[1];

    using this_type = sp_reactor_counted_impl_p<X>;

public:
    sp_reactor_counted_impl_p( sp_reactor_counted_impl_p const & ) = delete;
    sp_reactor_counted_impl_p & operator= ( sp_reactor_counted_impl_p const & ) = delete;

    explicit sp_reactor_counted_impl_p(int cpu, int cpu_num, X * px ):
        sp_reactor_counted_base(cpu),
        px_( px )
    {
        initialize(cpu_num, proxies);
    }

    virtual void dispose() noexcept
    {
        boost::checked_delete( px_ );
    }

    virtual void * get_deleter( boost::detail::sp_typeinfo const & )
    {
        return 0;
    }

    virtual void * get_untyped_deleter()
    {
        return 0;
    }

    static this_type* create(int cpu_num, X * px )
    {
        void* mem = malloc(sizeof(this_type) + (sizeof(sp_local_counted_base*) * (cpu_num - 1)));
        return new (mem) this_type(cpu_num, px);
    }

    virtual void set_local_counter(int cpu, sp_local_counted_base* counter) {
        proxies[cpu] = counter;
    }

    virtual sp_local_counted_base* get_local_counter(int cpu) {
        return proxies[cpu];
    }
};



template<class P, class D>
class sp_reactor_counted_impl_pd: public sp_reactor_counted_base
{
    P ptr; // copy constructor must not throw
    D del; // copy constructor must not throw

    sp_local_counted_base* proxies[1];

    using this_type = sp_reactor_counted_impl_pd<P, D> ;

public:
    sp_reactor_counted_impl_pd( sp_reactor_counted_impl_pd const & ) = delete;
    sp_reactor_counted_impl_pd & operator= ( sp_reactor_counted_impl_pd const & ) = delete;

    // pre: d(p) must not throw

    sp_reactor_counted_impl_pd(int cpu, int cpu_num, P p, D & d ):
        sp_reactor_counted_base(cpu),
        ptr( p ), del( d )
    {
        initialize(cpu_num, proxies);
    }

    sp_reactor_counted_impl_pd(int cpu, int cpu_num, P p ):
        sp_reactor_counted_base(cpu),
        ptr( p ), del()
    {
        initialize(cpu_num, proxies);
    }

    virtual void dispose() noexcept
    {
        del( ptr );
    }

    virtual void * get_deleter( boost::detail::sp_typeinfo const & ti )
    {
        return ti == BOOST_SP_TYPEID(D)? &reinterpret_cast<char&>( del ): 0;
    }

    virtual void * get_untyped_deleter()
    {
        return &reinterpret_cast<char&>( del );
    }

    static this_type* create(int cpu, int cpu_num, P p, D & d )
    {
        void* mem = malloc(sizeof(this_type) + (sizeof(sp_local_counted_base*) * (cpu_num - 1)));
        return new (mem) this_type(cpu, cpu_num, p, d);
    }


    static this_type* create(int cpu, int cpu_num, P p )
    {
        void* mem = malloc(sizeof(this_type) + (sizeof(sp_local_counted_base*) * (cpu_num - 1)));
        return new (mem) this_type(cpu, cpu_num, p);
    }

    virtual void set_local_counter(int cpu, sp_local_counted_base* counter) {
        proxies[cpu] = counter;
    }

    virtual sp_local_counted_base* get_local_counter(int cpu) {
        return proxies[cpu];
    }
};

} // namespace _

}}}
