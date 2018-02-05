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

    sp_local_counted_base * pi_;

    friend class local_weak_count;

public:

    local_shared_count() noexcept: pi_(nullptr)
    {
    }

    template<class Y>
    explicit local_shared_count( Y * p ): pi_( nullptr )
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

    template<class P, class D>
    local_shared_count( P p, D d ): pi_(nullptr)
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

    template< class P, class D >
    local_shared_count( P p, sp_inplace_tag<D> ): pi_( nullptr )
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











    template<class Y, class D>
    explicit local_shared_count( std::unique_ptr<Y, D> & r ): pi_( nullptr )
    {
        typedef typename sp_convert_reference<D>::type D2;

        D2 d2( r.get_deleter() );
        pi_ = new sp_counted_impl_pd< typename std::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );

        if(!pi_)
        {
            boost::throw_exception( std::bad_alloc() );
        }

        r.release();
    }

    ~local_shared_count() noexcept
    {
        if(pi_) {
            pi_->release();
        }
    }

    local_shared_count(local_shared_count const & r) noexcept : pi_(r.pi_)
    {
        if(pi_) {
            pi_->add_ref_copy();
        }
    }


    local_shared_count(local_shared_count && r) noexcept : pi_(r.pi_)
    {
        r.pi_ = nullptr;
    }


    explicit local_shared_count(shared_count const & r) noexcept
    {
        if(r.pi_)
        {
            auto pi = r.pi_->get_local_counter(cpu());

            if (pi) {
                pi_ = pi;
                pi_->add_ref_copy();
            }
            else {
                pi_ = new sp_local_counter(cpu(), r.pi_);
                r.pi_->set_local_counter(cpu(), pi_);
                r.pi_->add_ref_copy();
            }
        }
        else {
           pi_ = nullptr;
        }
    }

    explicit local_shared_count(shared_count&& r) noexcept
    {
        if(r.pi_)
        {
            auto pi = r.pi_->get_local_counter(cpu());

            if (pi)
            {
                pi_ = pi;
                pi_->add_ref_copy();
            }
            else {
                pi_ = new sp_local_counter(cpu(), r.pi_);
                r.pi_->set_local_counter(cpu(), pi_);
                r.pi_->add_ref_copy();
            }

            r.pi_->release();
            r.pi_ = nullptr;
        }
        else {
           pi_ = nullptr;
        }
    }

    explicit local_shared_count(local_weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0
    local_shared_count( local_weak_count const & r, sp_nothrow_tag ); // constructs an empty *this when r.use_count() == 0

    local_shared_count & operator= (local_shared_count const & r) noexcept
    {
        sp_local_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if( tmp) tmp->add_ref_copy();
            if( pi_) pi_->release();
            pi_ = tmp;
        }

        return *this;
    }

//    local_shared_count & operator= (shared_count const & r) noexcept
//    {
//        if (pi_) {
//            pi_->release();
//        }

//        sp_local_counted_base * tmp = r.pi_;

//        if( tmp != pi_ )
//        {
//            if( tmp) tmp->add_ref_copy();
//            if( pi_) pi_->release();
//            pi_ = tmp;
//        }

//        return *this;
//    }

    void swap(local_shared_count & r) noexcept
    {
        sp_local_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const noexcept
    {
        return pi_? pi_->use_count(): 0;
    }

    bool unique() const noexcept
    {
        return use_count() == 1;
    }

    bool empty() const noexcept
    {
        return pi_ == nullptr;
    }

    friend inline bool operator==(local_shared_count const & a, local_shared_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(local_shared_count const & a, local_shared_count const & b)
    {
        return std::less<sp_local_counted_base *>()( a.pi_, b.pi_ );
    }

    void * get_deleter( boost::detail::sp_typeinfo const & ti ) const
    {
        return pi_? pi_->get_deleter( ti ): 0;
    }

    void * get_untyped_deleter() const
    {
        return pi_? pi_->get_untyped_deleter(): 0;
    }

    static int cpu_num();
    static int cpu();
};


class local_weak_count
{
private:

    sp_local_counted_base * pi_;

    friend class local_shared_count;

public:

    local_weak_count() noexcept: pi_(nullptr)
    {
    }

    explicit local_weak_count(local_shared_count const & r) noexcept: pi_(r.pi_)
    {
        if(pi_) pi_->weak_add_ref();
    }

    local_weak_count(local_weak_count const & r) noexcept: pi_(r.pi_)
    {
        if(pi_) pi_->weak_add_ref();
    }

    local_weak_count(local_weak_count && r) noexcept: pi_(r.pi_)
    {
        r.pi_ = nullptr;
    }

    explicit local_weak_count(weak_count const & r) noexcept
    {
        if(r.pi_)
        {
            auto pi = r.pi_->get_local_counter(cpu());

            if (pi) {
                pi_ = pi;
                pi_->weak_add_ref();
            }
            else {
                pi_ = new sp_local_counter(cpu(), r.pi_);
                r.pi_->set_local_counter(cpu(), pi_);
                r.pi_->weak_add_ref();
            }
        }
        else {
           pi_ = nullptr;
        }
    }

    explicit local_weak_count(shared_count const & r) noexcept
    {
        if(r.pi_)
        {
            auto pi = r.pi_->get_local_counter(cpu());

            if (pi) {
                pi_ = pi;
                pi_->weak_add_ref();
            }
            else {
                pi_ = new sp_local_counter(cpu(), r.pi_);
                r.pi_->set_local_counter(cpu(), pi_);
                r.pi_->weak_add_ref();
            }
        }
        else {
           pi_ = nullptr;
        }
    }

    explicit local_weak_count(shared_count && r) noexcept
    {
        if(r.pi_)
        {
            auto pi = r.pi_->get_local_counter(cpu());

            if (pi) {
                pi_ = pi;
                pi_->weak_add_ref();
            }
            else {
                pi_ = new sp_local_counter(cpu(), r.pi_);
                r.pi_->set_local_counter(cpu(), pi_);
                r.pi_->weak_add_ref();
            }
        }
        else {
           pi_ = nullptr;
        }
    }


    explicit local_weak_count(weak_count && r) noexcept
    {
        if(r.pi_)
        {
            auto pi = r.pi_->get_local_counter(cpu());

            if (pi)
            {
                pi_ = pi;
                pi_->weak_add_ref();
            }
            else {
                pi_ = new sp_local_counter(cpu(), r.pi_);
                r.pi_->set_local_counter(cpu(), pi_);
                r.pi_->weak_add_ref();
            }
        }
        else {
           pi_ = nullptr;
        }
    }


    ~local_weak_count() noexcept
    {
        if(pi_) {
            pi_->weak_release();
        }
    }

    local_weak_count & operator= (local_shared_count const & r) noexcept
    {
        sp_local_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp) tmp->weak_add_ref();
            if(pi_) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    local_weak_count & operator= (local_weak_count const & r) noexcept
    {
        sp_local_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp) tmp->weak_add_ref();
            if(pi_) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(local_weak_count & r) noexcept
    {
        sp_local_counted_base * tmp = r.pi_;
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

    friend inline bool operator==(local_weak_count const & a, local_weak_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(local_weak_count const & a, local_weak_count const & b)
    {
        return std::less<sp_local_counted_base *>()(a.pi_, b.pi_);
    }

    static int cpu_num() {return local_shared_count::cpu_num();}
    static int cpu() {return local_shared_count::cpu();}
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




