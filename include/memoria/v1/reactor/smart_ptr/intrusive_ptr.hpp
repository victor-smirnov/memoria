
//
//  intrusive_ptr.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.
//

#pragma once

#include <boost/config.hpp>

#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_convertible.hpp>


#include <boost/config/no_tr1/functional.hpp>           // for std::less

#if !defined(BOOST_NO_IOSTREAM)
#if !defined(BOOST_NO_IOSFWD)
#include <iosfwd>               // for std::basic_ostream
#else
#include <ostream>
#endif
#endif


namespace memoria {
namespace v1 {
namespace reactor {

//
//  intrusive_ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//  
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//          (p != 0)
//
//  The object is responsible for destroying itself.
//

template<class T> class intrusive_ptr
{
private:

    typedef intrusive_ptr this_type;

public:

    typedef T element_type;

    constexpr intrusive_ptr() noexcept : px( 0 )
    {
    }

    intrusive_ptr( T * p, bool add_ref = true ): px( p )
    {
        if( px != 0 && add_ref ) intrusive_ptr_add_ref( px );
    }



    template<class U>
#if !defined( MMA1_SP_NO_SP_CONVERTIBLE )

    intrusive_ptr( intrusive_ptr<U> const & rhs, typename reactor::detail::sp_enable_if_convertible<U,T>::type = reactor::detail::sp_empty() )

#else

    intrusive_ptr( intrusive_ptr<U> const & rhs )

#endif
    : px( rhs.get() )
    {
        if( px != 0 ) intrusive_ptr_add_ref( px );
    }



    intrusive_ptr(intrusive_ptr const & rhs): px( rhs.px )
    {
        if( px != 0 ) intrusive_ptr_add_ref( px );
    }

    ~intrusive_ptr()
    {
        if( px != 0 ) intrusive_ptr_release( px );
    }


    template<class U> intrusive_ptr & operator=(intrusive_ptr<U> const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }



// Move support



    intrusive_ptr(intrusive_ptr && rhs) noexcept : px( rhs.px )
    {
        rhs.px = 0;
    }

    intrusive_ptr & operator=(intrusive_ptr && rhs) noexcept
    {
        this_type( static_cast< intrusive_ptr && >( rhs ) ).swap(*this);
        return *this;
    }

    template<class U> friend class intrusive_ptr;

    template<class U>
#if !defined( MMA1_SP_NO_SP_CONVERTIBLE )

    intrusive_ptr(intrusive_ptr<U> && rhs, typename reactor::detail::sp_enable_if_convertible<U,T>::type = reactor::detail::sp_empty())

#else

    intrusive_ptr(intrusive_ptr<U> && rhs)

#endif        
    : px( rhs.px )
    {
        rhs.px = 0;
    }

    template<class U>
    intrusive_ptr & operator=(intrusive_ptr<U> && rhs) noexcept
    {
        this_type( static_cast< intrusive_ptr<U> && >( rhs ) ).swap(*this);
        return *this;
    }



    intrusive_ptr & operator=(intrusive_ptr const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr & operator=(T * rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    void reset()
    {
        this_type().swap( *this );
    }

    void reset( T * rhs )
    {
        this_type( rhs ).swap( *this );
    }

    void reset( T * rhs, bool add_ref )
    {
        this_type( rhs, add_ref ).swap( *this );
    }

    T * get() const noexcept
    {
        return px;
    }

    T * detach() noexcept
    {
        T * ret = px;
        px = 0;
        return ret;
    }

    T & operator*() const noexcept
    {
        BOOST_ASSERT( px != 0 );
        return *px;
    }

    T * operator->() const noexcept
    {
        BOOST_ASSERT( px != 0 );
        return px;
    }

// implicit conversion to "bool"
#include <memoria/v1/reactor/smart_ptr/detail/operator_bool.hpp>

    void swap(intrusive_ptr & rhs) noexcept
    {
        T * tmp = px;
        px = rhs.px;
        rhs.px = tmp;
    }

private:

    T * px;
};

template<class T, class U> inline bool operator==(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b) noexcept
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b) noexcept
{
    return a.get() != b.get();
}

template<class T, class U> inline bool operator==(intrusive_ptr<T> const & a, U * b) noexcept
{
    return a.get() == b;
}

template<class T, class U> inline bool operator!=(intrusive_ptr<T> const & a, U * b) noexcept
{
    return a.get() != b;
}

template<class T, class U> inline bool operator==(T * a, intrusive_ptr<U> const & b) noexcept
{
    return a == b.get();
}

template<class T, class U> inline bool operator!=(T * a, intrusive_ptr<U> const & b) noexcept
{
    return a != b.get();
}




template<class T> inline bool operator==( intrusive_ptr<T> const & p, std::nullptr_t ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator==( std::nullptr_t, intrusive_ptr<T> const & p ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator!=( intrusive_ptr<T> const & p, std::nullptr_t ) noexcept
{
    return p.get() != 0;
}

template<class T> inline bool operator!=( std::nullptr_t, intrusive_ptr<T> const & p ) noexcept
{
    return p.get() != 0;
}



template<class T> inline bool operator<(intrusive_ptr<T> const & a, intrusive_ptr<T> const & b) noexcept
{
    return std::less<T *>()(a.get(), b.get());
}

template<class T> void swap(intrusive_ptr<T> & lhs, intrusive_ptr<T> & rhs) noexcept
{
    lhs.swap(rhs);
}

// mem_fn support

template<class T> T * get_pointer(intrusive_ptr<T> const & p) noexcept
{
    return p.get();
}

template<class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const & p)
{
    return static_cast<T *>(p.get());
}

template<class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const & p)
{
    return const_cast<T *>(p.get());
}

template<class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const & p)
{
    return dynamic_cast<T *>(p.get());
}

// operator<<

#if !defined(BOOST_NO_IOSTREAM)

#if defined(BOOST_NO_TEMPLATED_IOSTREAMS) || ( defined(__GNUC__) &&  (__GNUC__ < 3) )

template<class Y> std::ostream & operator<< (std::ostream & os, intrusive_ptr<Y> const & p)
{
    os << p.get();
    return os;
}

#else

// in STLport's no-iostreams mode no iostream symbols can be used
#ifndef _STLP_NO_IOSTREAMS

# if defined(BOOST_MSVC) && BOOST_WORKAROUND(BOOST_MSVC, < 1300 && __SGI_STL_PORT)
// MSVC6 has problems finding std::basic_ostream through the using declaration in namespace _STL
using std::basic_ostream;
template<class E, class T, class Y> basic_ostream<E, T> & operator<< (basic_ostream<E, T> & os, intrusive_ptr<Y> const & p)
# else
template<class E, class T, class Y> std::basic_ostream<E, T> & operator<< (std::basic_ostream<E, T> & os, intrusive_ptr<Y> const & p)
# endif 
{
    os << p.get();
    return os;
}

#endif // _STLP_NO_IOSTREAMS

#endif // __GNUC__ < 3

#endif // !defined(BOOST_NO_IOSTREAM)

// hash_value

template< class T > struct hash;

template< class T > std::size_t hash_value( reactor::intrusive_ptr<T> const & p ) noexcept
{
    return boost::hash< T* >()( p.get() );
}

}}}

