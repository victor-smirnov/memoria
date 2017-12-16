
//
//  weak_ptr.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov
//  Copyright 2017 Victor Smirnov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/weak_ptr.htm for documentation.
//

#pragma once

#include <memory> // boost.TR1 include order fix

#include <memoria/v1/reactor/smart_ptr/detail/local_shared_count.hpp>
#include <memoria/v1/reactor/smart_ptr/local_shared_ptr.hpp>

namespace memoria {
namespace v1 {
namespace reactor {


template<class T> class local_weak_ptr
{
private:

    // Borland 5.5.1 specific workarounds
    typedef local_weak_ptr<T> this_type;

public:

    typedef typename boost::detail::sp_element< T >::type element_type;

    local_weak_ptr() noexcept : px(0), pn() // never throws in 1.30+
    {
    }

//  generated copy constructor, assignment, destructor are fine...



// ... except in C++0x, move disables the implicit copy

    local_weak_ptr( local_weak_ptr const & r ) noexcept : px( r.px ), pn( r.pn )
    {
    }

    local_weak_ptr & operator=( local_weak_ptr const & r ) noexcept
    {
        px = r.px;
        pn = r.pn;
        return *this;
    }


//
//  The "obvious" converting constructor implementation:
//
//  template<class Y>
//  weak_ptr(weak_ptr<Y> const & r): px(r.px), pn(r.pn) // never throws
//  {
//  }
//
//  has a serious problem.
//
//  r.px may already have been invalidated. The px(r.px)
//  conversion may require access to *r.px (virtual inheritance).
//
//  It is not possible to avoid spurious access violations since
//  in multithreaded programs r.px may be invalidated at any point.
//

    template<class Y>


    local_weak_ptr( local_weak_ptr<Y> const & r, typename _::sp_enable_if_convertible<Y,T>::type = boost::detail::sp_empty() ) noexcept :
        px(r.lock().get()), pn(r.pn)
    {
        _::sp_assert_convertible< Y, T >();
    }


    template<class Y>

    local_weak_ptr( local_weak_ptr<Y> && r, typename _::sp_enable_if_convertible<Y,T>::type = _::sp_empty() ) noexcept:
        px( r.lock().get() ), pn( static_cast< _::local_weak_count && >( r.pn ) )
    {
        _::sp_assert_convertible< Y, T >();
        r.px = 0;
    }

    // for better efficiency in the T == Y case
    local_weak_ptr( local_weak_ptr && r ) noexcept:
        px( r.px ), pn( static_cast< _::local_weak_count && >( r.pn ) )
    {
        r.px = 0;
    }

    // for better efficiency in the T == Y case
    local_weak_ptr & operator=( local_weak_ptr && r ) noexcept
    {
        this_type( static_cast< local_weak_ptr && >( r ) ).swap( *this );
        return *this;
    }




    template<class Y>
    local_weak_ptr( local_shared_ptr<Y> const & r, typename boost::detail::sp_enable_if_convertible<Y,T>::type = boost::detail::sp_empty() ) noexcept :
        px( r.px ), pn( r.pn )
    {
        _::sp_assert_convertible< Y, T >();
    }

    template<class Y>
    local_weak_ptr & operator=( local_weak_ptr<Y> const & r ) noexcept
    {
        _::sp_assert_convertible< Y, T >();

        px = r.lock().get();
        pn = r.pn;

        return *this;
    }

    template<class Y>
    local_weak_ptr & operator=( local_weak_ptr<Y> && r ) noexcept
    {
        this_type( static_cast< local_weak_ptr<Y> && >( r ) ).swap( *this );
        return *this;
    }


    template<class Y>
    local_weak_ptr & operator=( local_shared_ptr<Y> const & r ) noexcept
    {
        _::sp_assert_convertible< Y, T >();

        px = r.px;
        pn = r.pn;

        return *this;
    }

    local_shared_ptr<T> lock() const noexcept
    {
        return local_shared_ptr<T>( *this, _::sp_nothrow_tag() );
    }

    long use_count() const noexcept
    {
        return pn.use_count();
    }

    bool expired() const noexcept
    {
        return pn.use_count() == 0;
    }

    bool _empty() const // extension, not in std::weak_ptr
    {
        return pn.empty();
    }

    void reset() noexcept // never throws in 1.30+
    {
        this_type().swap(*this);
    }

    void swap(this_type & other) noexcept
    {
        std::swap(px, other.px);
        pn.swap(other.pn);
    }

    template<typename Y>
    void _internal_aliasing_assign(local_weak_ptr<Y> const & r, element_type * px2)
    {
        px = px2;
        pn = r.pn;
    }

    template<class Y> bool owner_before( local_weak_ptr<Y> const & rhs ) const noexcept
    {
        return pn < rhs.pn;
    }

    template<class Y> bool owner_before( local_shared_ptr<Y> const & rhs ) const noexcept
    {
        return pn < rhs.pn;
    }

// Tasteless as this may seem, making all members public allows member templates
// to work in the absence of member template friends. (Matthew Langston)

private:

    template<class Y> friend class local_weak_ptr;
    template<class Y> friend class local_shared_ptr;


    element_type * px;            // contained pointer
    _::local_weak_count pn;       // reference counter

};  // local_weak_ptr

template<class T, class U> inline bool operator<(local_weak_ptr<T> const & a, local_weak_ptr<U> const & b) noexcept
{
    return a.owner_before( b );
}

template<class T> void swap(local_weak_ptr<T> & a, local_weak_ptr<T> & b) noexcept
{
    a.swap(b);
}

}}}


