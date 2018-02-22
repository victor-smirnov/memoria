
#pragma once

//
//  detail/shared_count.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifdef __BORLANDC__
# pragma warn -8027     // Functions containing try are not expanded inline
#endif

#include <memoria/v1/reactor/smart_ptr/detail/sp_common.hpp>

#include <boost/config.hpp>
#include <boost/checked_delete.hpp>
#include <boost/throw_exception.hpp>
#include <memoria/v1/reactor/smart_ptr/bad_weak_ptr.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_counted_base.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_counted_impl.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_disable_deprecated.hpp>
#include <boost/detail/workaround.hpp>
// In order to avoid circular dependencies with Boost.TR1
// we make sure that our include of <memory> doesn't try to
// pull in the TR1 headers: that's why we use this header 
// rather than including <memory> directly:
#include <boost/config/no_tr1/memory.hpp>  // std::auto_ptr
#include <functional>       // std::less

#ifdef BOOST_NO_EXCEPTIONS
# include <new>              // std::bad_alloc
#endif

#include <boost/core/addressof.hpp>

#if defined( MMA1_SP_DISABLE_DEPRECATED )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace memoria {
namespace v1 {
namespace reactor {

namespace movelib
{

template< class T, class D > class unique_ptr;

} // namespace movelib

namespace detail
{

#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)

int const shared_count_id   = 0x2C35F101;
int const weak_count_id     = 0x298C38A4;

#endif

struct sp_internal_constructor_tag {};

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



class CpuValue{
    int32_t cpu_;
public:
    CpuValue(int32_t cpu): cpu_(cpu) {}
    int32_t cpu() {return cpu_;}
};

struct AllocTag {};

#ifdef MMA1_SP_ENABLE_DEBUG_HOOKS
#define MMA1_SP_SPC_INIT_DEBUG_HOOK , id_(shared_count_id)
#else
#define MMA1_SP_SPC_INIT_DEBUG_HOOK
#endif

class shared_count
{
private:

    sp_counted_base * pi_;

#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class weak_count;

public:

    constexpr shared_count(): pi_(0) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
    }

    constexpr explicit shared_count( sp_counted_base * pi ): pi_( pi ) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
    }

    template<class Y>
    explicit shared_count(CpuValue cpu, Y * p): pi_( nullptr )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        pi_ = run_at(cpu.cpu(), [&]() {
            try
            {
                return new sp_counted_impl_p<Y>( p );
            }
            catch(...)
            {
                boost::checked_delete( p );
                throw;
            }
        });
    }


    template<class P, class D>
    shared_count(CpuValue cpu, P p, D d):
        pi_(nullptr)
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        pi_ = run_at(cpu.cpu(), [&]() {
            try
            {
                return new sp_counted_impl_pd<P, D>(p, d);
            }
            catch(...)
            {
                d(p); // delete p
                throw;
            }
        });
    }



    template< class P, class D, class... Args >
    shared_count(sp_internal_constructor_tag, CpuValue cpu, P p, sp_inplace_tag<D>, Args&&... args):
        pi_( nullptr )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        using T = std::remove_pointer_t<std::remove_cv_t<P>>;

        pi_ = run_at(cpu.cpu(), [&]() {
            try
            {
                sp_counted_base* pi = new sp_counted_impl_pd< P, D >( p );

                D* pd = static_cast<D*>(pi->get_untyped_deleter());
                void * pv = pd->address();

                ::new( pv ) T( std::forward<Args>( args )... );

                pd->set_initialized();

                return pi;
            }
            catch( ... )
            {
                D::operator_fn( p ); // delete p
                throw;
            }
        });
    }

    template<class P, class D, class A>
    shared_count(CpuValue cpu, P p, D d, A a):
        pi_( nullptr )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        pi_ = run_at(cpu.cpu(), [&]() {
            using impl_type = sp_counted_impl_pda<P, D, A>;

            using A2 = typename std::allocator_traits<A>::template rebind_alloc< impl_type >;
            A2 a2( a );

            sp_counted_base* pi{nullptr};

            try
            {
                pi = a2.allocate( 1 );
                ::new( static_cast< void* >( pi ) ) impl_type( p, d, a );
                return pi;
            }
            catch(...)
            {
                d( p );

                if( pi )
                {
                    a2.deallocate( static_cast< impl_type* >( pi ), 1 );
                }

                throw;
            }
        });
    }



    template< class P, class D, class A, class... Args >
    shared_count(sp_internal_constructor_tag, AllocTag, CpuValue cpu, P p, sp_inplace_tag< D >, A a, Args&&... args ):
        pi_( nullptr )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        pi_ = run_at(cpu.cpu(), [&]() {
            using impl_type = sp_counted_impl_pda< P, D, A >;

            using A2 = typename std::allocator_traits<A>::template rebind_alloc< impl_type >;
            A2 a2( a );

            using T = std::remove_pointer_t<std::remove_cv_t<P>>;
            using A3 = typename std::allocator_traits<A>::template rebind_alloc<T>;
            A3 a3( a );

            sp_counted_base* pi{nullptr};
            try
            {
                auto pi = a2.allocate( 1 );
                ::new( static_cast< void* >( pi ) ) impl_type( p, a );

                D* pd = static_cast<D*>(pi->get_untyped_deleter());
                void * pv = pd->address();

                std::allocator_traits<A3>::construct(
                    a3,
                    static_cast< T* >( pv ),
                    std::forward<Args>( args )...
                );

                pd->set_initialized();

                return pi;
            }
            catch(...)
            {
                D::operator_fn( p );

                if(pi)
                {
                    a2.deallocate( static_cast< impl_type* >( pi ), 1 );
                }

                throw;
            }
        });
    }




    template<class Y, class D>
    explicit shared_count(CpuValue cpu, std::unique_ptr<Y, D> & r ): pi_( 0 )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        pi_ = run_at(cpu.cpu(), [&]() {
            typedef typename sp_convert_reference<D>::type D2;

            D2 d2( r.get_deleter() );
            sp_counted_base* pi = new sp_counted_impl_pd< typename std::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );

            r.release();

            return pi;
        });
    }


    template<class Y, class D>
    explicit shared_count(CpuValue cpu, reactor::movelib::unique_ptr<Y, D> & r ): pi_( 0 )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        pi_ = run_at(cpu.cpu(), [&]() {
            typedef typename sp_convert_reference<D>::type D2;

            D2 d2( r.get_deleter() );
            sp_counted_base* pi = new sp_counted_impl_pd< typename reactor::movelib::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );
            r.release();

            return pi;
        });
    }

    ~shared_count() // nothrow
    {
        if( pi_ != 0 ) pi_->release();
#ifdef MMA1_SP_ENABLE_DEBUG_HOOK
        id_ = 0;
#endif
    }

    shared_count(shared_count const & r): pi_(r.pi_) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        if( pi_ != 0 ) pi_->add_ref_copy();
    }

    shared_count(shared_count && r): pi_(r.pi_) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        r.pi_ = 0;
    }


    explicit shared_count(weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0
    shared_count( weak_count const & r, sp_nothrow_tag ); // constructs an empty *this when r.use_count() == 0

    shared_count & operator= (shared_count const & r) // nothrow
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

    void swap(shared_count & r) // nothrow
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

    friend inline bool operator==(shared_count const & a, shared_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(shared_count const & a, shared_count const & b)
    {
        return std::less<sp_counted_base *>()( a.pi_, b.pi_ );
    }

    void * get_deleter( boost::detail::sp_typeinfo const & ti ) const
    {
        return pi_? pi_->get_deleter( ti ): 0;
    }

    void * get_local_deleter( boost::detail::sp_typeinfo const & ti ) const
    {
        return pi_? pi_->get_local_deleter( ti ): 0;
    }

    void * get_untyped_deleter() const
    {
        return pi_? pi_->get_untyped_deleter(): 0;
    }


    template <typename Fn>
    static auto run_at(int cpu, Fn&& fn);
};


class weak_count
{
private:

    sp_counted_base * pi_;

#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class shared_count;

public:

    constexpr weak_count(): pi_(0) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
    }

    weak_count(shared_count const & r): pi_(r.pi_) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

    weak_count(weak_count const & r): pi_(r.pi_) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

// Move support


    weak_count(weak_count && r): pi_(r.pi_) // nothrow
        MMA1_SP_SPC_INIT_DEBUG_HOOK
    {
        r.pi_ = 0;
    }

    ~weak_count() // nothrow
    {
        if(pi_ != 0) pi_->weak_release();
#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)
        id_ = 0;
#endif
    }

    weak_count & operator= (shared_count const & r) // nothrow
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

    weak_count & operator= (weak_count const & r) // nothrow
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

    void swap(weak_count & r) // nothrow
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

    friend inline bool operator==(weak_count const & a, weak_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(weak_count const & a, weak_count const & b)
    {
        return std::less<sp_counted_base *>()(a.pi_, b.pi_);
    }
};

inline shared_count::shared_count( weak_count const & r ): pi_( r.pi_ )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
{
    if( pi_ == 0 || !pi_->add_ref_lock() )
    {
        boost::throw_exception( reactor::bad_weak_ptr() );
    }
}

inline shared_count::shared_count( weak_count const & r, sp_nothrow_tag ): pi_( r.pi_ )
        MMA1_SP_SPC_INIT_DEBUG_HOOK
{
    if( pi_ != 0 && !pi_->add_ref_lock() )
    {
        pi_ = 0;
    }
}

} // namespace detail

}}}

#if defined( MMA1_SP_DISABLE_DEPRECATED )
#pragma GCC diagnostic pop
#endif



