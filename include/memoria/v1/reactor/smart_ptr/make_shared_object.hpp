
//  make_shared_object.hpp
//
//  Copyright (c) 2007, 2008, 2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#pragma once

#include <boost/config.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <memoria/v1/reactor/smart_ptr/shared_ptr.hpp>

#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <cstddef>
#include <new>

namespace memoria {
namespace v1 {
namespace reactor {

namespace detail
{

template< std::size_t N, std::size_t A > struct sp_aligned_storage
{
    union type
    {
        char data_[ N ];
        typename boost::type_with_alignment< A >::type align_;
    };
};

template< class T > class sp_ms_deleter
{
private:

    typedef typename sp_aligned_storage< sizeof( T ), ::boost::alignment_of< T >::value >::type storage_type;

    bool initialized_;
    storage_type storage_;

private:

    void destroy() noexcept
    {
        if( initialized_ )
        {
            reinterpret_cast< T* >( storage_.data_ )->~T();
            initialized_ = false;
        }
    }

public:

    sp_ms_deleter(int32_t cpu) noexcept : initialized_( false )
    {
    }

    template<class A> explicit sp_ms_deleter(A const & ) noexcept : initialized_( false )
    {
    }

    // optimization: do not copy storage_
    sp_ms_deleter( sp_ms_deleter const & r) noexcept : initialized_( false )
    {
    }

    ~sp_ms_deleter() noexcept
    {
        destroy();
    }

    void operator()( T * ) noexcept
    {
        destroy();
    }

    static void operator_fn( T* ) noexcept // operator() can't be static
    {
    }

    void * address() noexcept
    {
        return storage_.data_;
    }

    void set_initialized() noexcept
    {
        initialized_ = true;
    }
};

template< class T, class A > class sp_as_deleter
{
private:

    typedef typename sp_aligned_storage< sizeof( T ), ::boost::alignment_of< T >::value >::type storage_type;

    storage_type storage_;
    A a_;
    bool initialized_;

private:

    void destroy() noexcept
    {
        if( initialized_ )
        {
            T * p = reinterpret_cast< T* >( storage_.data_ );
            std::allocator_traits<A>::destroy( a_, p );
            initialized_ = false;
        }
    }

public:

    sp_as_deleter(A const & a ) noexcept : a_( a ), initialized_( false )
    {
    }

    // optimization: do not copy storage_
    sp_as_deleter( sp_as_deleter const & r ) noexcept : a_( r.a_), initialized_( false )
    {
    }

    ~sp_as_deleter() noexcept
    {
        destroy();
    }

    void operator()( T * ) noexcept
    {
        destroy();
    }

    static void operator_fn( T* ) noexcept // operator() can't be static
    {
    }

    void * address() noexcept
    {
        return storage_.data_;
    }

    void set_initialized() noexcept
    {
        initialized_ = true;
    }
};

template< class T >
struct sp_if_not_array
{
    typedef reactor::shared_ptr< T > type;
};


template< class T >
struct sp_if_not_array< T[] >
{
};


template< class T, std::size_t N >
struct sp_if_not_array< T[N] >
{
};


} // namespace detail

#define MMA1_SP_MSD( T ) reactor::detail::sp_inplace_tag< reactor::detail::sp_ms_deleter< T > >()


// _noinit versions

template< class T >
typename reactor::detail::sp_if_not_array< T >::type make_shared_noinit_at(int32_t cpu)
{
    reactor::shared_ptr< T > pt(
        detail::sp_internal_constructor_tag(),
        static_cast< T* >( nullptr ),
        MMA1_SP_MSD( T )
    );

    reactor::detail::sp_ms_deleter< T > * pd = static_cast<reactor::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    T * pt2 = static_cast< T* >( pd->address() );

    reactor::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return reactor::shared_ptr< T >( pt, pt2 );
}

template< class T, class A >
typename reactor::detail::sp_if_not_array< T >::type allocate_shared_noinit_at(int32_t cpu, A const & a )
{
    reactor::shared_ptr< T > pt(
        detail::sp_internal_constructor_tag(),
        static_cast< T* >( nullptr ),
        MMA1_SP_MSD( T ),
        a
    );

    reactor::detail::sp_ms_deleter< T > * pd = static_cast<reactor::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    T * pt2 = static_cast< T* >( pd->address() );

    reactor::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return reactor::shared_ptr< T >( pt, pt2 );
}


// Variadic templates, rvalue reference

template< class T, class... Args >
typename reactor::detail::sp_if_not_array< T >::type make_shared_at(int cpu, Args && ... args )
{
    reactor::shared_ptr< T > pt(
        detail::sp_internal_constructor_tag(),
        cpu,
        static_cast< T* >( nullptr ),
        MMA1_SP_MSD( T ),
        std::forward<Args>(args)...
    );

    reactor::detail::sp_ms_deleter< T > * pd = static_cast<reactor::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    T * pt2 = static_cast< T* >( pd->address() );

    reactor::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return reactor::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class... Args >
typename reactor::detail::sp_if_not_array< T >::type allocate_shared_at(int cpu, A const & a, Args && ... args )
{
    using A2 = typename std::allocator_traits<A>::template rebind_alloc<T>;
    A2 a2( a );

    using D = reactor::detail::sp_as_deleter< T, A2 >;

    reactor::shared_ptr< T > pt(
        detail::sp_internal_constructor_tag(),
        detail::AllocTag(),
        cpu,
        static_cast< T* >( nullptr ),
        reactor::detail::sp_inplace_tag<D>(),
        a2,
        std::forward<Args>(args)...
    );

    D * pd = static_cast< D* >( pt._internal_get_untyped_deleter() );
    T * pt2 = static_cast< T* >( pd->address() );

    reactor::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return reactor::shared_ptr< T >( pt, pt2 );
}


#undef MMA1_SP_MSD

}}}

