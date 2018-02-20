#ifndef MMA1_SMART_PTR_SCOPED_PTR_HPP_INCLUDED
#define MMA1_SMART_PTR_SCOPED_PTR_HPP_INCLUDED

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/checked_delete.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_nullptr_t.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_disable_deprecated.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_noexcept.hpp>
#include <boost/detail/workaround.hpp>

#ifndef BOOST_NO_AUTO_PTR
# include <memory>          // for std::auto_ptr
#endif

#if defined( BOOST_SP_DISABLE_DEPRECATED )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace memoria {
namespace v1 {
namespace reactor {

// Debug hooks

#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)

void sp_scalar_constructor_hook(void * p);
void sp_scalar_destructor_hook(void * p);

#endif

//  scoped_ptr mimics a built-in pointer except that it guarantees deletion
//  of the object pointed to, either on destruction of the scoped_ptr or via
//  an explicit reset(). scoped_ptr is a simple solution for simple needs;
//  use shared_ptr or std::auto_ptr if your needs are more complex.

template<class T> class scoped_ptr // noncopyable
{
private:

    T * px;

    scoped_ptr(scoped_ptr const &);
    scoped_ptr & operator=(scoped_ptr const &);

    typedef scoped_ptr<T> this_type;

    void operator==( scoped_ptr const& ) const;
    void operator!=( scoped_ptr const& ) const;

public:

    typedef T element_type;

    explicit scoped_ptr( T * p = 0 ) noexcept : px( p )
    {
#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)
        reactor::sp_scalar_constructor_hook( px );
#endif
    }

#ifndef BOOST_NO_AUTO_PTR

    explicit scoped_ptr( std::auto_ptr<T> p ) noexcept : px( p.release() )
    {
#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)
        reactor::sp_scalar_constructor_hook( px );
#endif
    }

#endif

    ~scoped_ptr() noexcept
    {
#if defined(MMA1_SP_ENABLE_DEBUG_HOOKS)
        reactor::sp_scalar_destructor_hook( px );
#endif
        boost::checked_delete( px );
    }

    void reset(T * p = 0) noexcept_WITH_ASSERT
    {
        BOOST_ASSERT( p == 0 || p != px ); // catch self-reset errors
        this_type(p).swap(*this);
    }

    T & operator*() const noexcept_WITH_ASSERT
    {
        BOOST_ASSERT( px != 0 );
        return *px;
    }

    T * operator->() const noexcept_WITH_ASSERT
    {
        BOOST_ASSERT( px != 0 );
        return px;
    }

    T * get() const noexcept
    {
        return px;
    }

// implicit conversion to "bool"
#include <memoria/v1/reactor/smart_ptr/detail/operator_bool.hpp>

    void swap(scoped_ptr & b) noexcept
    {
        T * tmp = b.px;
        b.px = px;
        px = tmp;
    }
};

#if !defined( BOOST_NO_CXX11_NULLPTR )

template<class T> inline bool operator==( scoped_ptr<T> const & p, reactor::detail::sp_nullptr_t ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator==( reactor::detail::sp_nullptr_t, scoped_ptr<T> const & p ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator!=( scoped_ptr<T> const & p, reactor::detail::sp_nullptr_t ) noexcept
{
    return p.get() != 0;
}

template<class T> inline bool operator!=( reactor::detail::sp_nullptr_t, scoped_ptr<T> const & p ) noexcept
{
    return p.get() != 0;
}

#endif

template<class T> inline void swap(scoped_ptr<T> & a, scoped_ptr<T> & b) noexcept
{
    a.swap(b);
}

// get_pointer(p) is a generic way to say p.get()

template<class T> inline T * get_pointer(scoped_ptr<T> const & p) noexcept
{
    return p.get();
}

}}}

#if defined( BOOST_SP_DISABLE_DEPRECATED )
#pragma GCC diagnostic pop
#endif

#endif // #ifndef MMA1_SMART_PTR_SCOPED_PTR_HPP_INCLUDED
