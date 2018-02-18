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
#include <memoria/v1/reactor/smart_ptr/detail/sp_counted_base_std_atomic.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_counted_impl.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_reactor_counted_impl.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_disable_deprecated.hpp>

#include <memoria/v1/core/config.hpp>

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

struct sp_nothrow_tag {};

template< class D > struct sp_inplace_tag
{
};




template< class T > class sp_reference_wrapper
{ 
public:

    explicit sp_reference_wrapper( T & t): t_( boost::addressof( t ) )
    {
    }

    template< class Y > void operator()( Y * p ) const
    {
        (*t_)( p );
    }

private:

    T * t_;
};

template< class D > struct sp_convert_reference
{
    typedef D type;
};

template< class D > struct sp_convert_reference< D& >
{
    typedef sp_reference_wrapper< D > type;
};

class weak_count;

class shared_count
{
private:

    sp_reactor_counted_base * pi_;

    friend class weak_count;
    friend class local_weak_count;
    friend class local_shared_count;

public:

    shared_count() noexcept: pi_(0)
    {
    }



    template<class Y> explicit shared_count(int cpu, Y * p ): pi_( 0 )
    {
        try
        {
            pi_ = run_at(cpu, [&]{
                return sp_reactor_counted_impl_p<Y>::create(cpu, p);
            });

//            pi_ = new sp_local_counter(rr);
//            rr->set_local_counter(cpu, pi_);
        }
        catch(...)
        {
            boost::checked_delete( p );
            throw;
        }
    }





    template<class P, class D> shared_count(int cpu, P p, D d ): pi_(0)
    {
        try
        {
            pi_ = run_at(cpu, [&]{
                return sp_reactor_counted_impl_pd<P, D>::create(cpu, p, d);
            });

//            pi_ = new sp_local_counter(rr);
//            rr->set_local_counter(cpu, pi_);
        }
        catch(...)
        {
            d(p); // delete p
            throw;
        }
    }




    template< class P, class D, class... Args>
    shared_count(int cpu, P p, sp_inplace_tag<D>, Args&&... args): pi_( nullptr )
    {
        pi_ = run_at(cpu, [&]{
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

        //pi_ = new sp_local_counter(rr);
        //rr->set_local_counter(cpu, pi_);
    }











//    template<class Y, class D>
//    explicit shared_count(int cpu, std::unique_ptr<Y, D> & r ): pi_( 0 )
//    {
//        typedef typename sp_convert_reference<D>::type D2;

//        D2 d2( r.get_deleter() );
//        pi_ = new sp_counted_impl_pd< typename std::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );

//        if( pi_ == 0 )
//        {
//            boost::throw_exception( std::bad_alloc() );
//        }

//        r.release();
//    }




    ~shared_count() noexcept
    {
        if( pi_ ) {
            pi_->release();
        }
    }

    shared_count(shared_count const & r) noexcept: pi_(r.pi_)
    {
        if( pi_ ) {
            pi_->add_ref_copy();
        }
    }


    shared_count(shared_count && r) noexcept : pi_(r.pi_)
    {
        r.pi_ = nullptr;
    }

    explicit shared_count(weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0
    shared_count( weak_count const & r, sp_nothrow_tag ); // constructs an empty *this when r.use_count() == 0

    shared_count & operator= (shared_count const & r) noexcept
    {
        sp_reactor_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if( tmp ) tmp->add_ref_copy();
            if( pi_ ) pi_->release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(shared_count & r) noexcept
    {
        sp_reactor_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const noexcept
    {
        return pi_ ? pi_->use_count(): 0;
    }

    bool unique() const noexcept
    {
        return use_count() == 1;
    }

    bool empty() const noexcept
    {
        return pi_ == nullptr;
    }

    friend inline bool operator==(shared_count const & a, shared_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(shared_count const & a, shared_count const & b)
    {
        return std::less<sp_reactor_counted_base *>()( a.pi_, b.pi_ );
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


class weak_count
{
private:

    sp_reactor_counted_base * pi_;

    friend class shared_count;
    friend class local_shared_count;
    friend class local_weak_count;

public:

    weak_count() noexcept: pi_(nullptr)
    {
    }

    weak_count(shared_count const & r) noexcept: pi_(r.pi_)
    {
        if(pi_) pi_->weak_add_ref();
    }

    weak_count(weak_count const & r)noexcept : pi_(r.pi_)
    {
        if(pi_) pi_->weak_add_ref();
    }

    weak_count(weak_count && r)noexcept : pi_(r.pi_)
    {
        r.pi_ = nullptr;
    }


    ~weak_count() noexcept
    {
        if(pi_) {
            pi_->weak_release();
        }
    }

    weak_count & operator= (shared_count const & r) noexcept
    {
        sp_reactor_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp) tmp->weak_add_ref();
            if(pi_) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    weak_count & operator= (weak_count const & r) noexcept
    {
        sp_reactor_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp) tmp->weak_add_ref();
            if(pi_) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(weak_count & r) noexcept
    {
        sp_reactor_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const noexcept
    {
        return pi_? pi_->use_count(): 0;
    }

    bool empty() const noexcept
    {
        return pi_ == nullptr;
    }

    friend inline bool operator==(weak_count const & a, weak_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(weak_count const & a, weak_count const & b)
    {
        return std::less<sp_reactor_counted_base *>()(a.pi_, b.pi_);
    }
};

inline shared_count::shared_count( weak_count const & r ): pi_( r.pi_ )
{
    if( (!pi_) || !pi_->add_ref_lock() )
    {
        boost::throw_exception( boost::bad_weak_ptr() );
    }
}

inline shared_count::shared_count( weak_count const & r, sp_nothrow_tag ): pi_( r.pi_ )
{
    if( (!pi_) && !pi_->add_ref_lock() )
    {
        pi_ = nullptr;
    }
}

} // namespace _

}}}




