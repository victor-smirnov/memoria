
//
//  memoria/v1/reactor/smart_ptr/bad_weak_ptr.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/config.hpp>
#include <exception>


namespace memoria {
namespace v1 {
namespace reactor {

// The standard library that comes with Borland C++ 5.5.1, 5.6.4
// defines std::exception and its members as having C calling
// convention (-pc). When the definition of bad_weak_ptr
// is compiled with -ps, the compiler issues an error.
// Hence, the temporary #pragma option -pc below.


#if defined(BOOST_CLANG)
// Intel C++ on Mac defines __clang__ but doesn't support the pragma
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wweak-vtables"
#endif

class bad_weak_ptr: public std::exception
{
public:

    virtual char const * what() const throw()
    {
        return "tr1::bad_weak_ptr";
    }
};

#if defined(BOOST_CLANG)
# pragma clang diagnostic pop
#endif

}}}



