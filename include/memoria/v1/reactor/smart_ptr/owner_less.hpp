
//
//  owner_less.hpp
//
//  Copyright (c) 2008 Frank Mori Hess
//  Copyright (c) 2016 Peter Dimov
//  Copyright 2017 Victor Smirnov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/smart_ptr.htm for documentation.
//

#pragma once

namespace memoria {
namespace v1 {
namespace reactor {

template<class T = void> struct owner_less
{
    typedef bool result_type;
    typedef T first_argument_type;
    typedef T second_argument_type;

    template<class U, class V> bool operator()( U const & u, V const & v ) const
    {
        return u.owner_before( v );
    }
};

}}}
