#pragma once


//
//  detail/shared_count.hpp
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
#include <boost/throw_exception.hpp>


#include <memoria/v1/reactor/smart_ptr/bad_weak_ptr.hpp>

#include <memoria/v1/reactor/smart_ptr/detail/shared_count.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_counted_base_std_atomic.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_counted_impl.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_reactor_counted_impl.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_disable_deprecated.hpp>

#include <memoria/v1/core/types/types.hpp>

#include <boost/detail/workaround.hpp>

// In order to avoid circular dependencies with Boost.TR1
// we make sure that our include of <memory> doesn't try to
// pull in the TR1 headers: that's why we use this header 
// rather than including <memory> directly:
#include <boost/config/no_tr1/memory.hpp>   // std::auto_ptr
#include <functional>                       // std::less


#include <boost/core/addressof.hpp>


namespace memoria {
namespace v1 {
namespace reactor {


namespace _
{

class local_weak_count;

class local_shared_count
{
private:

    sp_counted_base * pi_;

    friend class local_weak_count;

public:

    local_shared_count(): pi_(0) // nothrow
    {
    }

    template<class Y> explicit local_shared_count( Y * p ): pi_( 0 )
    {
        try
        {
            pi_ = new sp_counted_impl_p<Y>( p );
        }
        catch(...)
        {
            boost::checked_delete( p );
            throw;
        }
    }

    template<class Y> explicit local_shared_count(int cpu, Y * p ): pi_( 0 )
    {
        try
        {
            auto rr = run_at(cpu, [&]{
                return sp_reactor_counted_impl_p<Y>::create(cpu, p);
            });

            pi_ = new sp_local_counter(rr);

            rr->set_local_counter(cpu, pi_);
        }
        catch(...)
        {
            boost::checked_delete( p );
            throw;
        }
    }



    template<class P, class D> local_shared_count( P p, D d ): pi_(0)
    {
        try
        {
            pi_ = new sp_counted_impl_pd<P, D>(p, d);
        }
        catch(...)
        {
            d(p); // delete p
            throw;
        }
    }

    template<class P, class D> local_shared_count(int cpu, P p, D d ): pi_(0)
    {
        try
        {
            auto rr = run_at(cpu, [&]{
                return sp_reactor_counted_impl_pd<P, D>::create(cpu, p, d);
            });

            pi_ = new sp_local_counter(rr);

            rr->set_local_counter(cpu, pi_);
        }
        catch(...)
        {
            d(p); // delete p
            throw;
        }
    }



    template< class P, class D > local_shared_count( P p, sp_inplace_tag<D> ): pi_( 0 )
    {
        try
        {
            pi_ = new sp_counted_impl_pd< P, D >( p );
        }
        catch( ... )
        {
            D::operator_fn( p ); // delete p
            throw;
        }
    }


//    template< class P, class D > shared_count(int cpu, P p, sp_inplace_tag<D> ): pi_( 0 )
//    {
//        try
//        {
//            auto rr = run_at(cpu, [&]{
//                return sp_reactor_counted_impl_pd<P, D>::create(cpu, p);
//            });

//            pi_ = new sp_local_counter(rr);

//            rr->set_local_counter(cpu, pi_);
//        }
//        catch( ... )
//        {
//            D::operator_fn( p ); // delete p
//            throw;
//        }
//    }

    template< class P, class D, class... Args>
    local_shared_count(int cpu, P p, sp_inplace_tag<D>, Args&&... args): pi_( nullptr )
    {
        auto rr = run_at(cpu, [&]{
            auto sp = sp_reactor_counted_impl_pd<P, D>::create(cpu, cpu_num(), p);

            try {
                D* pd = static_cast<D*>(sp->get_untyped_deleter());
                void * pv = pd->address();
                ::new( pv ) std::remove_pointer_t<std::remove_cv_t<P>>( std::forward<Args>( args )... );
                pd->set_initialized(cpu);
            }
            catch (...) {
                sp->destroy();
                throw;
            }

            return sp;
        });

        pi_ = new sp_local_counter(rr);

        rr->set_local_counter(cpu, pi_);
    }



    template<class P, class D, class A> local_shared_count( P p, D d, A a ): pi_( 0 )
    {
        typedef sp_counted_impl_pda<P, D, A> impl_type;

        typedef typename std::allocator_traits<A>::template rebind_alloc< impl_type > A2;

        A2 a2( a );

        try
        {
            pi_ = a2.allocate( 1 );
            ::new( static_cast< void* >( pi_ ) ) impl_type( p, d, a );
        }
        catch(...)
        {
            d( p );

            if( pi_ != 0 )
            {
                a2.deallocate( static_cast< impl_type* >( pi_ ), 1 );
            }

            throw;
        }
    }


    template< class P, class D, class A > local_shared_count( P p, sp_inplace_tag< D >, A a ): pi_( 0 )
    {
        typedef sp_counted_impl_pda< P, D, A > impl_type;

        typedef typename std::allocator_traits<A>::template rebind_alloc< impl_type > A2;

        A2 a2( a );

        try
        {
            pi_ = a2.allocate( 1 );
            ::new( static_cast< void* >( pi_ ) ) impl_type( p, a );
        }
        catch(...)
        {
            D::operator_fn( p );

            if( pi_ != 0 )
            {
                a2.deallocate( static_cast< impl_type* >( pi_ ), 1 );
            }

            throw;
        }

    }





    template<class Y, class D>
    explicit local_shared_count( std::unique_ptr<Y, D> & r ): pi_( 0 )
    {
        typedef typename sp_convert_reference<D>::type D2;

        D2 d2( r.get_deleter() );
        pi_ = new sp_counted_impl_pd< typename std::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );

        if( pi_ == 0 )
        {
            boost::throw_exception( std::bad_alloc() );
        }

        r.release();
    }


    template<class Y, class D>
    explicit local_shared_count( movelib::unique_ptr<Y, D> & r ): pi_( 0 )
    {
        typedef typename sp_convert_reference<D>::type D2;

        D2 d2( r.get_deleter() );
        pi_ = new sp_counted_impl_pd< typename movelib::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );

        if( pi_ == 0 )
        {
            boost::throw_exception( std::bad_alloc() );
        }

        r.release();
    }

    ~local_shared_count() noexcept
    {
        if( pi_ != 0 ) pi_->release();
    }

    local_shared_count(local_shared_count const & r) noexcept : pi_(r.pi_)
    {
        if( pi_ != 0 ) pi_->add_ref_copy();
    }


    local_shared_count(local_shared_count && r) noexcept : pi_(r.pi_) // nothrow
    {
        r.pi_ = 0;
    }

    explicit local_shared_count(local_weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0
    local_shared_count( local_weak_count const & r, sp_nothrow_tag ); // constructs an empty *this when r.use_count() == 0

    local_shared_count & operator= (local_shared_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if( tmp != 0 ) tmp->add_ref_copy();
            if( pi_ != 0 ) pi_->release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(local_shared_count & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const // nothrow
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    bool unique() const // nothrow
    {
        return use_count() == 1;
    }

    bool empty() const // nothrow
    {
        return pi_ == 0;
    }

    friend inline bool operator==(local_shared_count const & a, local_shared_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(local_shared_count const & a, local_shared_count const & b)
    {
        return std::less<sp_counted_base *>()( a.pi_, b.pi_ );
    }

    void * get_deleter( boost::detail::sp_typeinfo const & ti ) const
    {
        return pi_? pi_->get_deleter( ti ): 0;
    }

    void * get_untyped_deleter() const
    {
        return pi_? pi_->get_untyped_deleter(): 0;
    }

private:
    template <typename Fn>
    static auto run_at(int cpu, Fn&& fn);

    static int cpu_num();
};


class local_weak_count
{
private:

    sp_counted_base * pi_;

    friend class local_shared_count;

public:

    local_weak_count(): pi_(0) // nothrow
    {
    }

    local_weak_count(local_shared_count const & r): pi_(r.pi_) // nothrow
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

    local_weak_count(local_weak_count const & r): pi_(r.pi_) // nothrow
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

// Move support


    local_weak_count(local_weak_count && r): pi_(r.pi_) // nothrow
    {
        r.pi_ = 0;
    }


    ~local_weak_count() // nothrow
    {
        if(pi_ != 0) pi_->weak_release();
    }

    local_weak_count & operator= (local_shared_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp != 0) tmp->weak_add_ref();
            if(pi_ != 0) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    local_weak_count & operator= (local_weak_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp != 0) tmp->weak_add_ref();
            if(pi_ != 0) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(local_weak_count & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const // nothrow
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    bool empty() const // nothrow
    {
        return pi_ == 0;
    }

    friend inline bool operator==(local_weak_count const & a, local_weak_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(local_weak_count const & a, local_weak_count const & b)
    {
        return std::less<sp_counted_base *>()(a.pi_, b.pi_);
    }
};

inline local_shared_count::local_shared_count( local_weak_count const & r ): pi_( r.pi_ )
{
    if( pi_ == 0 || !pi_->add_ref_lock() )
    {
        boost::throw_exception( boost::bad_weak_ptr() );
    }
}

inline local_shared_count::local_shared_count( local_weak_count const & r, sp_nothrow_tag ): pi_( r.pi_ )
{
    if( pi_ != 0 && !pi_->add_ref_lock() )
    {
        pi_ = 0;
    }
}

} // namespace _

}}}




