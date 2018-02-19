#ifndef MMA1_SMART_PTR_DETAIL_LOCAL_SP_DELETER_HPP_INCLUDED
#define MMA1_SMART_PTR_DETAIL_LOCAL_SP_DELETER_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/local_sp_deleter.hpp
//
//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <memoria/v1/reactor/smart_ptr/detail/local_counted_base.hpp>
#include <boost/config.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

namespace detail
{

template<class D> class local_sp_deleter: public local_counted_impl_em
{
private:

    D d_;

public:

    local_sp_deleter(): d_()
    {
    }

    explicit local_sp_deleter( D const& d ) noexcept: d_( d )
    {
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    explicit local_sp_deleter( D&& d ) noexcept: d_( std::move(d) )
    {
    }

#endif

    D& deleter()
    {
        return d_;
    }

    template<class Y> void operator()( Y* p ) noexcept
    {
        d_( p );
    }

#if !defined( BOOST_NO_CXX11_NULLPTR )

    void operator()( reactor::detail::sp_nullptr_t p ) noexcept
    {
        d_( p );
    }

#endif
};

template<> class local_sp_deleter<void>
{
};

template<class D> D * get_local_deleter( local_sp_deleter<D> * p )
{
    return &p->deleter();
}

inline void * get_local_deleter( local_sp_deleter<void> * /*p*/ )
{
    return 0;
}

} // namespace detail

}}}

#endif  // #ifndef MMA1_SMART_PTR_DETAIL_LOCAL_SP_DELETER_HPP_INCLUDED
