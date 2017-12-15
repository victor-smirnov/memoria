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

template<class X>
class sp_counted_impl_p: public sp_counted_base
{
    X * px_;

    using this_type = sp_counted_impl_p<X>;

public:
    sp_counted_impl_p( sp_counted_impl_p const & ) = delete;
    sp_counted_impl_p & operator= ( sp_counted_impl_p const & ) = delete;

    explicit sp_counted_impl_p( X * px ): px_( px )
    {}

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


    void * operator new( std::size_t )
    {
        return std::allocator<this_type>().allocate( 1, static_cast<this_type *>(0) );
    }

    void operator delete( void * p )
    {
        std::allocator<this_type>().deallocate( static_cast<this_type *>(p), 1 );
    }
};



template<class P, class D>
class sp_counted_impl_pd: public sp_counted_base
{
    P ptr; // copy constructor must not throw
    D del; // copy constructor must not throw

    using this_type = sp_counted_impl_pd<P, D> ;

public:
    sp_counted_impl_pd( sp_counted_impl_pd const & ) = delete;
    sp_counted_impl_pd & operator= ( sp_counted_impl_pd const & ) = delete;

    // pre: d(p) must not throw

    sp_counted_impl_pd( P p, D & d ): ptr( p ), del( d )
    {
    }

    sp_counted_impl_pd( P p ): ptr( p ), del()
    {
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



    void * operator new( std::size_t )
    {
        return std::allocator<this_type>().allocate( 1, static_cast<this_type *>(0) );
    }

    void operator delete( void * p )
    {
        std::allocator<this_type>().deallocate( static_cast<this_type *>(p), 1 );
    }
};

template<class P, class D, class A>
class sp_counted_impl_pda: public sp_counted_base
{
    P p_; // copy constructor must not throw
    D d_; // copy constructor must not throw
    A a_; // copy constructor must not throw

    using this_type = sp_counted_impl_pda<P, D, A> ;

public:
    sp_counted_impl_pda( sp_counted_impl_pda const & ) = delete;
    sp_counted_impl_pda & operator= ( sp_counted_impl_pda const & ) = delete;

    // pre: d( p ) must not throw

    sp_counted_impl_pda( P p, D & d, A a ): p_( p ), d_( d ), a_( a )
    {
    }

    sp_counted_impl_pda( P p, A a ): p_( p ), d_( a ), a_( a )
    {
    }

    virtual void dispose() noexcept
    {
        d_( p_ );
    }

    virtual void destroy() noexcept
    {
        typedef typename std::allocator_traits<A>::template rebind_alloc< this_type > A2;

        A2 a2( a_ );

        this->~this_type();

        a2.deallocate( this, 1 );
    }

    virtual void * get_deleter( boost::detail::sp_typeinfo const & ti )
    {
        return ti == BOOST_SP_TYPEID( D )? &reinterpret_cast<char&>( d_ ): 0;
    }

    virtual void * get_untyped_deleter()
    {
        return &reinterpret_cast<char&>( d_ );
    }
};




} // namespace detail

}}}
