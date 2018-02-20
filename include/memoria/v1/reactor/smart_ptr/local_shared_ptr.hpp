#ifndef MMA1_SMART_PTR_LOCAL_SHARED_PTR_HPP_INCLUDED
#define MMA1_SMART_PTR_LOCAL_SHARED_PTR_HPP_INCLUDED

//  local_shared_ptr.hpp
//
//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <memoria/v1/reactor/smart_ptr/shared_ptr.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

template<class T> class local_shared_ptr;

namespace detail
{

template< class E, class Y > inline void lsp_pointer_construct( reactor::local_shared_ptr< E > * /*ppx*/, Y * p, reactor::detail::local_counted_base * & pn )
{
    reactor::detail::sp_assert_convertible< Y, E >();

    typedef reactor::detail::local_sp_deleter< boost::checked_deleter<Y> > D;

    reactor::shared_ptr<E> p2( p, D() );

    D * pd = static_cast< D * >( p2._internal_get_untyped_deleter() );

    pd->pn_ = p2._internal_count();

    pn = pd;
}

template< class E, class Y > inline void lsp_pointer_construct( reactor::local_shared_ptr< E[] > * /*ppx*/, Y * p, reactor::detail::local_counted_base * & pn )
{
    reactor::detail::sp_assert_convertible< Y[], E[] >();

    typedef reactor::detail::local_sp_deleter< boost::checked_array_deleter<E> > D;

    reactor::shared_ptr<E[]> p2( p, D() );

    D * pd = static_cast< D * >( p2._internal_get_untyped_deleter() );

    pd->pn_ = p2._internal_count();

    pn = pd;
}

template< class E, std::size_t N, class Y > inline void lsp_pointer_construct( reactor::local_shared_ptr< E[N] > * /*ppx*/, Y * p, reactor::detail::local_counted_base * & pn )
{
    reactor::detail::sp_assert_convertible< Y[N], E[N] >();

    typedef reactor::detail::local_sp_deleter< boost::checked_array_deleter<E> > D;

    reactor::shared_ptr<E[N]> p2( p, D() );

    D * pd = static_cast< D * >( p2._internal_get_untyped_deleter() );

    pd->pn_ = p2._internal_count();

    pn = pd;
}

template< class E, class P, class D >
inline void lsp_deleter_construct( reactor::local_shared_ptr< E > * /*ppx*/, P p, D const& d, reactor::detail::local_counted_base * & pn )
{
    typedef reactor::detail::local_sp_deleter<D> D2;

    reactor::shared_ptr<E> p2( p, D2( d ) );

    D2 * pd = static_cast< D2 * >( p2._internal_get_untyped_deleter() );

    pd->pn_ = p2._internal_count();

    pn = pd;
}

template< class E, class P, class D, class A >
inline void lsp_allocator_construct( reactor::local_shared_ptr< E > * /*ppx*/, P p, D const& d, A const& a, reactor::detail::local_counted_base * & pn )
{
    typedef reactor::detail::local_sp_deleter<D> D2;

    reactor::shared_ptr<E> p2( p, D2( d ), a );

    D2 * pd = static_cast< D2 * >( p2._internal_get_untyped_deleter() );

    pd->pn_ = p2._internal_count();

    pn = pd;
}

struct lsp_internal_constructor_tag
{
};

} // namespace detail

//
// local_shared_ptr
//
// as shared_ptr, but local to a thread.
// reference count manipulations are non-atomic.
//

template<class T> class local_shared_ptr
{
private:

    typedef local_shared_ptr this_type;

public:

    typedef typename reactor::detail::sp_element<T>::type element_type;

private:

    element_type * px;
    reactor::detail::local_counted_base * pn;

    template<class Y> friend class local_shared_ptr;

public:

    // destructor

    ~local_shared_ptr() noexcept
    {
        if( pn )
        {
            pn->release();
        }
    }

    // constructors

    constexpr local_shared_ptr() noexcept : px( 0 ), pn( 0 )
    {
    }

#if !defined( BOOST_NO_CXX11_NULLPTR )

    constexpr local_shared_ptr( reactor::detail::sp_nullptr_t ) noexcept : px( 0 ), pn( 0 )
    {
    }

#endif

    // internal constructor, used by make_shared
    constexpr local_shared_ptr( reactor::detail::lsp_internal_constructor_tag, element_type * px_, reactor::detail::local_counted_base * pn_ ) noexcept : px( px_ ), pn( pn_ )
    {
    }

    template<class Y>
    explicit local_shared_ptr( Y * p ): px( p ), pn( 0 )
    {
        reactor::detail::lsp_pointer_construct( this, p, pn );
    }

    template<class Y, class D> local_shared_ptr( Y * p, D d ): px( p ), pn( 0 )
    {
        reactor::detail::lsp_deleter_construct( this, p, d, pn );
    }

#if !defined( BOOST_NO_CXX11_NULLPTR )

    template<class D> local_shared_ptr( reactor::detail::sp_nullptr_t p, D d ): px( p ), pn( 0 )
    {
        reactor::detail::lsp_deleter_construct( this, p, d, pn );
    }

#endif

    template<class Y, class D, class A> local_shared_ptr( Y * p, D d, A a ): px( p ), pn( 0 )
    {
        reactor::detail::lsp_allocator_construct( this, p, d, a, pn );
    }

#if !defined( BOOST_NO_CXX11_NULLPTR )

    template<class D, class A> local_shared_ptr( reactor::detail::sp_nullptr_t p, D d, A a ): px( p ), pn( 0 )
    {
        reactor::detail::lsp_allocator_construct( this, p, d, a, pn );
    }

#endif

    // construction from shared_ptr

    template<class Y> local_shared_ptr( shared_ptr<Y> const & r,
        typename reactor::detail::sp_enable_if_convertible<Y, T>::type = reactor::detail::sp_empty() )
        : px( r.get() ), pn( 0 )
    {
        reactor::detail::sp_assert_convertible< Y, T >();

        if( r.use_count() != 0 )
        {
            pn = new reactor::detail::local_counted_impl( r._internal_count() );
        }
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    template<class Y> local_shared_ptr( shared_ptr<Y> && r,
        typename reactor::detail::sp_enable_if_convertible<Y, T>::type = reactor::detail::sp_empty() )
        : px( r.get() ), pn( 0 )
    {
        reactor::detail::sp_assert_convertible< Y, T >();

        if( r.use_count() != 0 )
        {
            pn = new reactor::detail::local_counted_impl( r._internal_count() );
            r.reset();
        }
    }

#endif

    // construction from unique_ptr

#if !defined( BOOST_NO_CXX11_SMART_PTR ) && !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    template< class Y, class D >
    local_shared_ptr( std::unique_ptr< Y, D > && r,
        typename reactor::detail::sp_enable_if_convertible<Y, T>::type = reactor::detail::sp_empty() )
        : px( r.get() ), pn( 0 )
    {
        reactor::detail::sp_assert_convertible< Y, T >();

        if( px )
        {
            pn = new reactor::detail::local_counted_impl( shared_ptr<T>( std::move(r) )._internal_count() );
        }
    }

#endif

    template< class Y, class D >
    local_shared_ptr( reactor::movelib::unique_ptr< Y, D > r ); // !
    //	: px( r.get() ), pn( new reactor::detail::local_counted_impl( shared_ptr<T>( std::move(r) ) ) )
    //{
    //	reactor::detail::sp_assert_convertible< Y, T >();
    //}

    // copy constructor

    local_shared_ptr( local_shared_ptr const & r ) noexcept : px( r.px ), pn( r.pn )
    {
        if( pn )
        {
            pn->add_ref();
        }
    }

    // move constructor

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    local_shared_ptr( local_shared_ptr && r ) noexcept : px( r.px ), pn( r.pn )
    {
        r.px = 0;
        r.pn = 0;
    }

#endif

    // converting copy constructor

    template<class Y> local_shared_ptr( local_shared_ptr<Y> const & r,
        typename reactor::detail::sp_enable_if_convertible<Y, T>::type = reactor::detail::sp_empty() ) noexcept
        : px( r.px ), pn( r.pn )
    {
        reactor::detail::sp_assert_convertible< Y, T >();

        if( pn )
        {
            pn->add_ref();
        }
    }

    // converting move constructor

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    template<class Y> local_shared_ptr( local_shared_ptr<Y> && r,
        typename reactor::detail::sp_enable_if_convertible<Y, T>::type = reactor::detail::sp_empty() ) noexcept
        : px( r.px ), pn( r.pn )
    {
        reactor::detail::sp_assert_convertible< Y, T >();

        r.px = 0;
        r.pn = 0;
    }

#endif

    // aliasing

    template<class Y>
    local_shared_ptr( local_shared_ptr<Y> const & r, element_type * p ) noexcept : px( p ), pn( r.pn )
    {
        if( pn )
        {
            pn->add_ref();
        }
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    template<class Y>
    local_shared_ptr( local_shared_ptr<Y> && r, element_type * p ) noexcept : px( p ), pn( r.pn )
    {
        r.px = 0;
        r.pn = 0;
    }

#endif

    // assignment

    local_shared_ptr & operator=( local_shared_ptr const & r ) noexcept
    {
        local_shared_ptr( r ).swap( *this );
        return *this;
    }

    template<class Y> local_shared_ptr & operator=( local_shared_ptr<Y> const & r ) noexcept
    {
        local_shared_ptr( r ).swap( *this );
        return *this;
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    local_shared_ptr & operator=( local_shared_ptr && r ) noexcept
    {
        local_shared_ptr( std::move( r ) ).swap( *this );
        return *this;
    }

    template<class Y>
    local_shared_ptr & operator=( local_shared_ptr<Y> && r ) noexcept
    {
        local_shared_ptr( std::move( r ) ).swap( *this );
        return *this;
    }

#endif

#if !defined( BOOST_NO_CXX11_NULLPTR )

    local_shared_ptr & operator=( reactor::detail::sp_nullptr_t ) noexcept
    {
        local_shared_ptr().swap(*this);
        return *this;
    }

#endif

#if !defined( BOOST_NO_CXX11_SMART_PTR ) && !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    template<class Y, class D>
    local_shared_ptr & operator=( std::unique_ptr<Y, D> && r )
    {
        local_shared_ptr( std::move(r) ).swap( *this );
        return *this;
    }

#endif

    template<class Y, class D>
    local_shared_ptr & operator=( reactor::movelib::unique_ptr<Y, D> r ); // !

    // reset

    void reset() noexcept
    {
        local_shared_ptr().swap( *this );
    }

    template<class Y> void reset( Y * p ) // Y must be complete
    {
        local_shared_ptr( p ).swap( *this );
    }

    template<class Y, class D> void reset( Y * p, D d )
    {
        local_shared_ptr( p, d ).swap( *this );
    }

    template<class Y, class D, class A> void reset( Y * p, D d, A a )
    {
        local_shared_ptr( p, d, a ).swap( *this );
    }

    template<class Y> void reset( local_shared_ptr<Y> const & r, element_type * p ) noexcept
    {
        local_shared_ptr( r, p ).swap( *this );
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    template<class Y> void reset( local_shared_ptr<Y> && r, element_type * p ) noexcept
    {
        local_shared_ptr( std::move( r ), p ).swap( *this );
    }

#endif

    // accessors

    typename reactor::detail::sp_dereference< T >::type operator* () const noexcept
    {
        return *px;
    }

    typename reactor::detail::sp_member_access< T >::type operator-> () const noexcept
    {
        return px;
    }

    typename reactor::detail::sp_array_access< T >::type operator[] ( std::ptrdiff_t i ) const noexcept_WITH_ASSERT
    {
        BOOST_ASSERT( px != 0 );
        BOOST_ASSERT( i >= 0 && ( i < reactor::detail::sp_extent< T >::value || reactor::detail::sp_extent< T >::value == 0 ) );

        return static_cast< typename reactor::detail::sp_array_access< T >::type >( px[ i ] );
    }

    element_type * get() const noexcept
    {
        return px;
    }

    // implicit conversion to "bool"
#include <memoria/v1/reactor/smart_ptr/detail/operator_bool.hpp>

    long local_use_count() const noexcept
    {
        return pn? pn->local_use_count(): 0;
    }

    // conversions to shared_ptr, weak_ptr

#if !defined( BOOST_SP_NO_SP_CONVERTIBLE ) && !defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)
    template<class Y, class E = typename reactor::detail::sp_enable_if_convertible<T,Y>::type> operator shared_ptr<Y>() const noexcept
#else
    template<class Y> operator shared_ptr<Y>() const noexcept
#endif
    {
        reactor::detail::sp_assert_convertible<T, Y>();

        if( pn )
        {
            return shared_ptr<Y>( reactor::detail::sp_internal_constructor_tag(), px, pn->local_cb_get_shared_count() );
        }
        else
        {
            return shared_ptr<Y>();
        }
    }

#if !defined( BOOST_SP_NO_SP_CONVERTIBLE ) && !defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)
    template<class Y, class E = typename reactor::detail::sp_enable_if_convertible<T,Y>::type> operator weak_ptr<Y>() const noexcept
#else
    template<class Y> operator weak_ptr<Y>() const noexcept
#endif
    {
        reactor::detail::sp_assert_convertible<T, Y>();

        if( pn )
        {
            return shared_ptr<Y>( reactor::detail::sp_internal_constructor_tag(), px, pn->local_cb_get_shared_count() );
        }
        else
        {
            return weak_ptr<Y>();
        }
    }

    // swap

    void swap( local_shared_ptr & r ) noexcept
    {
        std::swap( px, r.px );
        std::swap( pn, r.pn );
    }

    // owner_before

    template<class Y> bool owner_before( local_shared_ptr<Y> const & r ) const noexcept
    {
        return std::less< reactor::detail::local_counted_base* >()( pn, r.pn );
    }
};

template<class T, class U> inline bool operator==( local_shared_ptr<T> const & a, local_shared_ptr<U> const & b ) noexcept
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=( local_shared_ptr<T> const & a, local_shared_ptr<U> const & b ) noexcept
{
    return a.get() != b.get();
}

#if !defined( BOOST_NO_CXX11_NULLPTR )

template<class T> inline bool operator==( local_shared_ptr<T> const & p, reactor::detail::sp_nullptr_t ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator==( reactor::detail::sp_nullptr_t, local_shared_ptr<T> const & p ) noexcept
{
    return p.get() == 0;
}

template<class T> inline bool operator!=( local_shared_ptr<T> const & p, reactor::detail::sp_nullptr_t ) noexcept
{
    return p.get() != 0;
}

template<class T> inline bool operator!=( reactor::detail::sp_nullptr_t, local_shared_ptr<T> const & p ) noexcept
{
    return p.get() != 0;
}

#endif

template<class T, class U> inline bool operator==( local_shared_ptr<T> const & a, shared_ptr<U> const & b ) noexcept
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=( local_shared_ptr<T> const & a, shared_ptr<U> const & b ) noexcept
{
    return a.get() != b.get();
}

template<class T, class U> inline bool operator==( shared_ptr<T> const & a, local_shared_ptr<U> const & b ) noexcept
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=( shared_ptr<T> const & a, local_shared_ptr<U> const & b ) noexcept
{
    return a.get() != b.get();
}

template<class T, class U> inline bool operator<(local_shared_ptr<T> const & a, local_shared_ptr<U> const & b) noexcept
{
    return a.owner_before( b );
}

template<class T> inline void swap( local_shared_ptr<T> & a, local_shared_ptr<T> & b ) noexcept
{
    a.swap( b );
}

template<class T, class U> local_shared_ptr<T> static_pointer_cast( local_shared_ptr<U> const & r ) noexcept
{
    (void) static_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = static_cast< E* >( r.get() );
    return local_shared_ptr<T>( r, p );
}

template<class T, class U> local_shared_ptr<T> const_pointer_cast( local_shared_ptr<U> const & r ) noexcept
{
    (void) const_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = const_cast< E* >( r.get() );
    return local_shared_ptr<T>( r, p );
}

template<class T, class U> local_shared_ptr<T> dynamic_pointer_cast( local_shared_ptr<U> const & r ) noexcept
{
    (void) dynamic_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = dynamic_cast< E* >( r.get() );
    return p? local_shared_ptr<T>( r, p ): local_shared_ptr<T>();
}

template<class T, class U> local_shared_ptr<T> reinterpret_pointer_cast( local_shared_ptr<U> const & r ) noexcept
{
    (void) reinterpret_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = reinterpret_cast< E* >( r.get() );
    return local_shared_ptr<T>( r, p );
}



template<class T, class U> local_shared_ptr<T> static_pointer_cast( local_shared_ptr<U> && r ) noexcept
{
    (void) static_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = static_cast< E* >( r.get() );
    return local_shared_ptr<T>( std::move(r), p );
}

template<class T, class U> local_shared_ptr<T> const_pointer_cast( local_shared_ptr<U> && r ) noexcept
{
    (void) const_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = const_cast< E* >( r.get() );
    return local_shared_ptr<T>( std::move(r), p );
}

template<class T, class U> local_shared_ptr<T> dynamic_pointer_cast( local_shared_ptr<U> && r ) noexcept
{
    (void) dynamic_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = dynamic_cast< E* >( r.get() );
    return p? local_shared_ptr<T>( std::move(r), p ): local_shared_ptr<T>();
}

template<class T, class U> local_shared_ptr<T> reinterpret_pointer_cast( local_shared_ptr<U> && r ) noexcept
{
    (void) reinterpret_cast< T* >( static_cast< U* >( 0 ) );

    typedef typename local_shared_ptr<T>::element_type E;

    E * p = reinterpret_cast< E* >( r.get() );
    return local_shared_ptr<T>( std::move(r), p );
}



// get_pointer() enables reactor::mem_fn to recognize local_shared_ptr

template<class T> inline typename local_shared_ptr<T>::element_type * get_pointer( local_shared_ptr<T> const & p ) noexcept
{
    return p.get();
}

// operator<<

#if !defined(BOOST_NO_IOSTREAM)

template<class E, class T, class Y> std::basic_ostream<E, T> & operator<< ( std::basic_ostream<E, T> & os, local_shared_ptr<Y> const & p )
{
    os << p.get();
    return os;
}

#endif // !defined(BOOST_NO_IOSTREAM)

// get_deleter

template<class D, class T> D * get_deleter( local_shared_ptr<T> const & p ) noexcept
{
    return get_deleter<D>( shared_ptr<T>( p ) );
}

}}}

namespace boost {

// hash_value

template< class T > struct hash;

template< class T > std::size_t hash_value( memoria::v1::reactor::local_shared_ptr<T> const & p ) noexcept
{
    return boost::hash< typename memoria::v1::reactor::local_shared_ptr<T>::element_type* >()( p.get() );
}

}

#endif  // #ifndef MMA1_SMART_PTR_LOCAL_SHARED_PTR_HPP_INCLUDED
