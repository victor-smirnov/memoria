
//  make_shared_object.hpp
//
//  Copyright (c) 2007, 2008, 2012 Peter Dimov
//  Copyright 2017 Victor Smirnov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/make_shared.html
//  for documentation.

#pragma once

#include <boost/config.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <cstddef>
#include <new>
#include <memory>

#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/reactor/smart_ptr/shared_ptr.hpp>

namespace memoria {
namespace v1 {
namespace reactor_1_64 {


namespace _
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

    void destroy()
    {
        if( initialized_ )
        {
            T* p = tools::ptr_cast<T>(storage_.data_);
            p->~T();
            initialized_ = false;
        }
    }

public:

    sp_ms_deleter() noexcept : initialized_( false )
    {
    }

    template<class A> explicit sp_ms_deleter( A const & ) noexcept : initialized_( false )
    {
    }

    // optimization: do not copy storage_
    sp_ms_deleter( sp_ms_deleter const & ) noexcept : initialized_( false )
    {
    }

    ~sp_ms_deleter()
    {
        destroy();
    }

    void operator()( T * )
    {
        destroy();
    }

    static void operator_fn( T* ) // operator() can't be static
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



template< class T > class sp_reactor_ms_deleter
{
private:

    typedef typename sp_aligned_storage< sizeof( T ), ::boost::alignment_of< T >::value >::type storage_type;

    int cpu_{};
    bool initialized_{};
    storage_type storage_;

private:

    void destroy();

public:

    sp_reactor_ms_deleter() noexcept {}

    template<class A> explicit
    sp_reactor_ms_deleter(A const & ) noexcept {}

    // optimization: do not copy storage_
    sp_reactor_ms_deleter( sp_reactor_ms_deleter const & other) noexcept {}

    ~sp_reactor_ms_deleter()
    {
        destroy();
    }

    void operator()( T * )
    {
        destroy();
    }

    static void operator_fn( T* ) // operator() can't be static
    {
    }

    void * address() noexcept
    {
        return storage_.data_;
    }

    void set_initialized(int cpu) noexcept
    {
        cpu_ = cpu;
        initialized_ = true;
    }
};



template< class T > struct sp_if_not_array
{
    typedef shared_ptr< T > type;
};


template< class T > struct sp_if_not_array< T[] >
{
};


template< class T, std::size_t N > struct sp_if_not_array< T[N] >
{
};

template< class T > struct local_sp_if_not_array
{
    typedef local_shared_ptr< T > type;
};


template< class T > struct local_sp_if_not_array< T[] >
{
};


template< class T, std::size_t N > struct local_sp_if_not_array< T[N] >
{
};

} // namespace _

# define BOOST_LOCAL_SP_MSD( T ) _::sp_inplace_tag< _::sp_ms_deleter< T > >()
# define BOOST_SP_MSD( T ) _::sp_inplace_tag< _::sp_reactor_ms_deleter< T > >()

// Variadic templates, rvalue reference

template< class T, class... Args > typename _::local_sp_if_not_array< T >::type make_local_shared( Args && ... args )
{
    static_assert(std::is_base_of<enable_local_shared_from_this<T>, T>::value, "");

    local_shared_ptr< T > pt( static_cast<T*>(nullptr), BOOST_LOCAL_SP_MSD( T ) );

    _::sp_ms_deleter< T > * pd = static_cast<_::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T( std::forward<Args>( args )... );
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    _::sp_enable_local_shared_from_this( &pt, pt2, pt2 );
    return local_shared_ptr< T >( pt, pt2 );
}


template< class T, class... Args > typename _::sp_if_not_array< T >::type make_shared(int cpu, Args && ... args )
{
    static_assert(std::is_base_of<enable_shared_from_this<T>, T>::value, "");

    shared_ptr<T> pt(cpu, static_cast<T*>(nullptr), BOOST_SP_MSD( T ), std::forward<Args>(args)...);

    _::sp_ms_deleter< T > * pd = static_cast<_::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    T * pt2 = static_cast< T* >( pv );

    _::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return shared_ptr< T >( pt, pt2 );
}




#undef BOOST_LOCAL_SP_MSD
#undef BOOST_SP_MSD

}}}

