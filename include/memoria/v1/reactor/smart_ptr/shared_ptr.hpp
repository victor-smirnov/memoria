
//
//  shared_ptr.hpp
//
//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001-2008 Peter Dimov
//  Copyright 2017 Victor Smirnov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/shared_ptr.htm for documentation.
//

#pragma once

#include <boost/config.hpp>   // for broken compiler workarounds

// In order to avoid circular dependencies with Boost.TR1
// we make sure that our include of <memory> doesn't try to
// pull in the TR1 headers: that's why we use this header 
// rather than including <memory> directly:
#include <boost/config/no_tr1/memory.hpp>  // std::auto_ptr

#include <boost/assert.hpp>
#include <boost/checked_delete.hpp>
#include <boost/throw_exception.hpp>

#include <memoria/v1/reactor/smart_ptr/detail/shared_count.hpp>

#include <boost/detail/workaround.hpp>

#include <memoria/v1/reactor/smart_ptr/detail/sp_convertible.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_disable_deprecated.hpp>


#include <algorithm>            // for std::swap
#include <functional>           // for std::less
#include <typeinfo>             // for std::bad_cast
#include <cstddef>              // for std::size_t

#include <iosfwd>               // for std::basic_ostream


namespace memoria {
namespace v1 {
namespace reactor {


template<class T> class shared_ptr;
template<class T> class weak_ptr;
template<class T> class enable_shared_from_this;
class enable_shared_from_raw;

namespace movelib
{

    template< class T, class D > class unique_ptr;

} // namespace movelib

namespace _
{

// sp_element, element_type

template< class T > struct sp_element
{
    typedef T type;
};



template< class T > struct sp_element< T[] >
{
    typedef T type;
};



template< class T, std::size_t N > struct sp_element< T[N] >
{
    typedef T type;
};



// sp_dereference, return type of operator*

template< class T > struct sp_dereference
{
    typedef T & type;
};

template<> struct sp_dereference< void >
{
    typedef void type;
};



template<> struct sp_dereference< void const >
{
    typedef void type;
};

template<> struct sp_dereference< void volatile >
{
    typedef void type;
};

template<> struct sp_dereference< void const volatile >
{
    typedef void type;
};





template< class T > struct sp_dereference< T[] >
{
    typedef void type;
};



template< class T, std::size_t N > struct sp_dereference< T[N] >
{
    typedef void type;
};


// sp_member_access, return type of operator->

template< class T > struct sp_member_access
{
    typedef T * type;
};



template< class T > struct sp_member_access< T[] >
{
    typedef void type;
};

template< class T, std::size_t N > struct sp_member_access< T[N] >
{
    typedef void type;
};


// sp_array_access, return type of operator[]

template< class T > struct sp_array_access
{
    typedef void type;
};



template< class T > struct sp_array_access< T[] >
{
    typedef T & type;
};



template< class T, std::size_t N > struct sp_array_access< T[N] >
{
    typedef T & type;
};



// sp_extent, for operator[] index check

template< class T > struct sp_extent
{
    enum _vt { value = 0 };
};



template< class T, std::size_t N > struct sp_extent< T[N] >
{
    enum _vt { value = N };
};



// enable_shared_from_this support

template< class X, class Y, class T > inline void sp_enable_shared_from_this( shared_ptr<X> const * ppx, Y const * py, enable_shared_from_this< T > const * pe )
{
    if( pe != 0 )
    {
        pe->_internal_accept_owner( ppx, const_cast< Y* >( py ) );
    }
}

template< class X, class Y > inline void sp_enable_shared_from_this( shared_ptr<X> * ppx, Y const * py, enable_shared_from_raw const * pe );



inline void sp_enable_shared_from_this( ... )
{
}



// sp_assert_convertible

template< class Y, class T > inline void sp_assert_convertible()
{
    // static_assert( sp_convertible< Y, T >::value );
    typedef char tmp[ sp_convertible< Y, T >::value? 1: -1 ];
    (void)sizeof( tmp );
}

// pointer constructor helper

template< class T, class Y > inline void sp_pointer_construct( shared_ptr< T > * ppx, Y * p, _::shared_count & pn )
{
    _::shared_count( p ).swap( pn );
    _::sp_enable_shared_from_this( ppx, p, p );
}

template< class T, class Y > inline void sp_pointer_construct(int cpu, shared_ptr< T > * ppx, Y * p, _::shared_count & pn )
{
    _::shared_count(cpu, p ).swap( pn );
    _::sp_enable_shared_from_this( ppx, p, p );
}

template< class T, class Y > inline void sp_pointer_construct( shared_ptr< T[] > * /*ppx*/, Y * p, _::shared_count & pn )
{
    sp_assert_convertible< Y[], T[] >();
    _::shared_count( p, boost::checked_array_deleter< T >() ).swap( pn );
}

template< class T, class Y > inline void sp_pointer_construct(int cpu, shared_ptr< T[] > * /*ppx*/, Y * p, _::shared_count & pn )
{
    sp_assert_convertible< Y[], T[] >();
    _::shared_count(cpu, p, boost::checked_array_deleter< T >() ).swap( pn );
}


template< class T, std::size_t N, class Y >
inline void sp_pointer_construct( shared_ptr< T[N] > * /*ppx*/, Y * p, _::shared_count & pn )
{
    sp_assert_convertible< Y[N], T[N] >();
    _::shared_count( p, boost::checked_array_deleter< T >() ).swap( pn );
}

template< class T, std::size_t N, class Y >
inline void sp_pointer_construct(int cpu, shared_ptr< T[N] > * /*ppx*/, Y * p, _::shared_count & pn )
{
    sp_assert_convertible< Y[N], T[N] >();
    _::shared_count(cpu, p, boost::checked_array_deleter< T >() ).swap( pn );
}



// deleter constructor helper

template< class T, class Y > inline void sp_deleter_construct( shared_ptr< T > * ppx, Y * p )
{
    _::sp_enable_shared_from_this( ppx, p, p );
}


template< class T, class Y > inline void sp_deleter_construct( shared_ptr< T[] > * /*ppx*/, Y * /*p*/ )
{
    sp_assert_convertible< Y[], T[] >();
}

template< class T, std::size_t N, class Y >
inline void sp_deleter_construct( shared_ptr< T[N] > * /*ppx*/, Y * /*p*/ )
{
    sp_assert_convertible< Y[N], T[N] >();
}






} // namespace detail


//
//  shared_ptr
//
//  An enhanced relative of scoped_ptr with reference counted copy semantics.
//  The object pointed to is deleted when the last shared_ptr pointing to it
//  is destroyed or reset.
//

template<class T> class shared_ptr
{
private:

    // Borland 5.5.1 specific workaround
    typedef shared_ptr<T> this_type;

public:

    typedef typename _::sp_element< T >::type element_type;

    shared_ptr() noexcept : px( 0 ), pn() // never throws in 1.30+
    {
    }


    shared_ptr( std::nullptr_t ) noexcept : px( 0 ), pn() // never throws
    {
    }


    template<class Y>
    explicit shared_ptr( Y * p ): px( p ), pn() // Y must be complete
    {
        _::sp_pointer_construct( this, p, pn );
    }

    template<class Y>
    explicit shared_ptr(int cpu, Y * p ): px( p ), pn() // Y must be complete
    {
        _::sp_pointer_construct(cpu, this, p, pn );
    }

    //
    // Requirements: D's copy constructor must not throw
    //
    // shared_ptr will release p by calling d(p)
    //

    template<class Y, class D> shared_ptr( Y * p, D d ): px( p ), pn( p, d )
    {
        _::sp_deleter_construct( this, p );
    }

    template<class Y, class D, class... Args>
    shared_ptr(int cpu, Y * p, D d, Args&&... args):
        px( p ), pn(cpu, p, d, std::forward<Args>(args)...)
    {
        _::sp_deleter_construct( this, p );
    }


    template<class D> shared_ptr( std::nullptr_t p, D d ): px( p ), pn( p, d )
    {
    }

    // As above, but with allocator. A's copy constructor shall not throw.

    template<class Y, class D, class A> shared_ptr( Y * p, D d, A a ): px( p ), pn( p, d, a )
    {
        _::sp_deleter_construct( this, p );
    }


    template<class D, class A> shared_ptr( std::nullptr_t p, D d, A a ): px( p ), pn( p, d, a )
    {
    }



    shared_ptr( shared_ptr const & r ) noexcept : px( r.px ), pn( r.pn )
    {
    }


    template<class Y>
    explicit shared_ptr( weak_ptr<Y> const & r ): pn( r.pn ) // may throw
    {
        _::sp_assert_convertible< Y, T >();

        // it is now safe to copy r.px, as pn(r.pn) did not throw
        px = r.px;
    }

    template<class Y>
    shared_ptr( weak_ptr<Y> const & r, _::sp_nothrow_tag ) noexcept :
        px( 0 ), pn( r.pn, _::sp_nothrow_tag() )
    {
        if( !pn.empty() )
        {
            px = r.px;
        }
    }

    template<class Y>
    shared_ptr( shared_ptr<Y> const & r, typename _::sp_enable_if_convertible<Y,T>::type = _::sp_empty() )
    noexcept : px( r.px ), pn( r.pn )
    {
        _::sp_assert_convertible< Y, T >();
    }

    // aliasing
    template< class Y >
    shared_ptr( shared_ptr<Y> const & r, element_type * p ) noexcept : px( p ), pn( r.pn )
    {
    }



    template< class Y, class D >
    shared_ptr( std::unique_ptr< Y, D > && r ): px( r.get() ), pn()
    {
        _::sp_assert_convertible< Y, T >();

        typename std::unique_ptr< Y, D >::pointer tmp = r.get();
        pn = _::shared_count( r );

        _::sp_deleter_construct( this, tmp );
    }


    template< class Y, class D >
    shared_ptr( movelib::unique_ptr< Y, D > r ): px( r.get() ), pn()
    {
        _::sp_assert_convertible< Y, T >();

        typename movelib::unique_ptr< Y, D >::pointer tmp = r.get();
        pn = _::shared_count( r );

        _::sp_deleter_construct( this, tmp );
    }

    // assignment

    shared_ptr & operator=( shared_ptr const & r ) noexcept
    {
        this_type(r).swap(*this);
        return *this;
    }


    template<class Y>
    shared_ptr & operator=(shared_ptr<Y> const & r) noexcept
    {
        this_type(r).swap(*this);
        return *this;
    }



    template<class Y, class D>
    shared_ptr & operator=( std::unique_ptr<Y, D> && r )
    {
        this_type( static_cast< std::unique_ptr<Y, D> && >( r ) ).swap(*this);
        return *this;
    }


    template<class Y, class D>
    shared_ptr & operator=( movelib::unique_ptr<Y, D> r )
    {
        // this_type( static_cast< unique_ptr<Y, D> && >( r ) ).swap( *this );

        _::sp_assert_convertible< Y, T >();

        typename movelib::unique_ptr< Y, D >::pointer p = r.get();

        shared_ptr tmp;

        tmp.px = p;
        tmp.pn = _::shared_count( r );

        _::sp_deleter_construct( &tmp, p );

        tmp.swap( *this );

        return *this;
    }

// Move support


    shared_ptr( shared_ptr && r ) noexcept : px( r.px ), pn()
    {
        pn.swap( r.pn );
        r.px = 0;
    }

    template<class Y>
    shared_ptr( shared_ptr<Y> && r, typename _::sp_enable_if_convertible<Y,T>::type = _::sp_empty() )
    noexcept : px( r.px ), pn()
    {
        _::sp_assert_convertible< Y, T >();

        pn.swap( r.pn );
        r.px = 0;
    }

    shared_ptr & operator=( shared_ptr && r ) noexcept
    {
        this_type( static_cast< shared_ptr && >( r ) ).swap( *this );
        return *this;
    }

    template<class Y>
    shared_ptr & operator=( shared_ptr<Y> && r ) noexcept
    {
        this_type( static_cast< shared_ptr<Y> && >( r ) ).swap( *this );
        return *this;
    }

    // aliasing move
    template<class Y>
    shared_ptr( shared_ptr<Y> && r, element_type * p ) noexcept : px( p ), pn()
    {
        pn.swap( r.pn );
        r.px = 0;
    }


    shared_ptr & operator=( std::nullptr_t ) noexcept // never throws
    {
        this_type().swap(*this);
        return *this;
    }

    void reset() noexcept // never throws in 1.30+
    {
        this_type().swap(*this);
    }

    template<class Y> void reset( Y * p ) // Y must be complete
    {
        BOOST_ASSERT( p == 0 || p != px ); // catch self-reset errors
        this_type( p ).swap( *this );
    }

    template<class Y, class D> void reset( Y * p, D d )
    {
        this_type( p, d ).swap( *this );
    }

    template<class Y, class D, class A> void reset( Y * p, D d, A a )
    {
        this_type( p, d, a ).swap( *this );
    }

    template<class Y> void reset( shared_ptr<Y> const & r, element_type * p )
    {
        this_type( r, p ).swap( *this );
    }

    template<class Y> void reset( shared_ptr<Y> && r, element_type * p )
    {
        this_type( static_cast< shared_ptr<Y> && >( r ), p ).swap( *this );
    }


    // never throws (but has a BOOST_ASSERT in it, so not marked with noexcept)
    typename _::sp_dereference< T >::type operator* () const
    {
        BOOST_ASSERT( px != 0 );
        return *px;
    }
    
    // never throws (but has a BOOST_ASSERT in it, so not marked with noexcept)
    typename _::sp_member_access< T >::type operator-> () const
    {
        BOOST_ASSERT( px != 0 );
        return px;
    }
    
    // never throws (but has a BOOST_ASSERT in it, so not marked with noexcept)
    typename _::sp_array_access< T >::type operator[] ( std::ptrdiff_t i ) const
    {
        BOOST_ASSERT( px != 0 );
        BOOST_ASSERT( i >= 0 && ( i < _::sp_extent< T >::value || _::sp_extent< T >::value == 0 ) );

        return static_cast< typename _::sp_array_access< T >::type >( px[ i ] );
    }

    element_type * get() const noexcept
    {
        return px;
    }

    bool operator! () const noexcept
    {
        return px == 0;
    }

    operator bool () const noexcept
    {
        return px != 0;
    }


    bool unique() const noexcept
    {
        return pn.unique();
    }

    long use_count() const noexcept
    {
        return pn.use_count();
    }

    void swap( shared_ptr & other ) noexcept
    {
        std::swap(px, other.px);
        pn.swap(other.pn);
    }

    template<class Y> bool owner_before( shared_ptr<Y> const & rhs ) const noexcept
    {
        return pn < rhs.pn;
    }

    template<class Y> bool owner_before( weak_ptr<Y> const & rhs ) const noexcept
    {
        return pn < rhs.pn;
    }

    void * _internal_get_deleter( boost::detail::sp_typeinfo const & ti ) const noexcept
    {
        return pn.get_deleter( ti );
    }

    void * _internal_get_untyped_deleter() const noexcept
    {
        return pn.get_untyped_deleter();
    }

    bool _internal_equiv( shared_ptr const & r ) const noexcept
    {
        return px == r.px && pn == r.pn;
    }


private:

    template<class Y> friend class shared_ptr;
    template<class Y> friend class weak_ptr;

    element_type * px;                 // contained pointer
    _::shared_count pn;           // reference counter

};  // shared_ptr

template<class T, class U> inline bool operator==(shared_ptr<T> const & a, shared_ptr<U> const & b) noexcept
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(shared_ptr<T> const & a, shared_ptr<U> const & b) noexcept
{
    return a.get() != b.get();
}



template<class T> inline bool operator==( shared_ptr<T> const & p, std::nullptr_t ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator==( std::nullptr_t, shared_ptr<T> const & p ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator!=( shared_ptr<T> const & p, std::nullptr_t ) noexcept
{
    return p.get() != 0;
}

template<class T> inline bool operator!=( std::nullptr_t, shared_ptr<T> const & p ) noexcept
{
    return p.get() != 0;
}


template<class T, class U> inline bool operator<(shared_ptr<T> const & a, shared_ptr<U> const & b) noexcept
{
    return a.owner_before( b );
}

template<class T> inline void swap(shared_ptr<T> & a, shared_ptr<T> & b) noexcept
{
    a.swap(b);
}

template<class T, class U> shared_ptr<T> static_pointer_cast( shared_ptr<U> const & r ) noexcept
{
    (void) static_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = static_cast< E* >( r.get() );
    return shared_ptr<T>( r, p );
}

template<class T, class U> shared_ptr<T> const_pointer_cast( shared_ptr<U> const & r ) noexcept
{
    (void) const_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = const_cast< E* >( r.get() );
    return shared_ptr<T>( r, p );
}

template<class T, class U> shared_ptr<T> dynamic_pointer_cast( shared_ptr<U> const & r ) noexcept
{
    (void) dynamic_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = dynamic_cast< E* >( r.get() );
    return p? shared_ptr<T>( r, p ): shared_ptr<T>();
}

template<class T, class U> shared_ptr<T> reinterpret_pointer_cast( shared_ptr<U> const & r ) noexcept
{
    (void) reinterpret_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = reinterpret_cast< E* >( r.get() );
    return shared_ptr<T>( r, p );
}


template<class T, class U> shared_ptr<T> static_pointer_cast( shared_ptr<U> && r ) noexcept
{
    (void) static_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = static_cast< E* >( r.get() );
    return shared_ptr<T>( std::move(r), p );
}

template<class T, class U> shared_ptr<T> const_pointer_cast( shared_ptr<U> && r ) noexcept
{
    (void) const_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = const_cast< E* >( r.get() );
    return shared_ptr<T>( std::move(r), p );
}

template<class T, class U> shared_ptr<T> dynamic_pointer_cast( shared_ptr<U> && r ) noexcept
{
    (void) dynamic_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = dynamic_cast< E* >( r.get() );
    return p? shared_ptr<T>( std::move(r), p ): shared_ptr<T>();
}

template<class T, class U> shared_ptr<T> reinterpret_pointer_cast( shared_ptr<U> && r ) noexcept
{
    (void) reinterpret_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename shared_ptr<T>::element_type E;

    E * p = reinterpret_cast< E* >( r.get() );
    return shared_ptr<T>( std::move(r), p );
}


// get_pointer() enables mem_fn to recognize shared_ptr

template<class T> inline typename shared_ptr<T>::element_type * get_pointer(shared_ptr<T> const & p) noexcept
{
    return p.get();
}

// operator<<


template<class Y> std::ostream & operator<< (std::ostream & os, shared_ptr<Y> const & p)
{
    os << p.get();
    return os;
}



// get_deleter

namespace _
{


template<class D, class T> D * basic_get_deleter( shared_ptr<T> const & p ) noexcept
{
    return static_cast<D *>( p._internal_get_deleter(BOOST_SP_TYPEID(D)) );
}


class esft2_deleter_wrapper
{
private:

    shared_ptr<void const volatile> deleter_;

public:

    esft2_deleter_wrapper()
    {
    }

    template< class T > void set_deleter( shared_ptr<T> const & deleter )
    {
        deleter_ = deleter;
    }

    template<typename D> D* get_deleter() const noexcept
    {
        return _::basic_get_deleter<D>( deleter_ );
    }

    template< class T> void operator()( T* )
    {
        BOOST_ASSERT( deleter_.use_count() <= 1 );
        deleter_.reset();
    }
};

} // namespace _

template<class D, class T> D * get_deleter( shared_ptr<T> const & p ) noexcept
{
    D *del = _::basic_get_deleter<D>(p);

    if(del == 0)
    {
        _::esft2_deleter_wrapper *del_wrapper = _::basic_get_deleter<_::esft2_deleter_wrapper>(p);
// The following get_deleter method call is fully qualified because
// older versions of gcc (2.95, 3.2.3) fail to compile it when written del_wrapper->get_deleter<D>()
        if(del_wrapper) del = del_wrapper->_::esft2_deleter_wrapper::get_deleter<D>();
    }

    return del;
}


// hash_value

template< class T > struct hash;

template< class T > std::size_t hash_value( shared_ptr<T> const & p ) noexcept
{
    return hash< typename shared_ptr<T>::element_type* >()( p.get() );
}

}}}

