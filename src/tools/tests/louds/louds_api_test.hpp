// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_
#define MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

class LoudsApiTest: public SPTestTask {

    typedef LoudsApiTest                                                       	MyType;

    typedef typename SCtrTF<LOUDS>::Type 										Ctr;

public:

    LoudsApiTest(): SPTestTask("API")
    {
        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~LoudsApiTest() throw () {}


    void runTest()
    {
    	Allocator allocator;

    	Ctr ctr(&allocator);
    }


};

}

#endif
