
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LOUDS_CREATE_HPP_
#define MEMORIA_TESTS_LOUDS_CREATE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LoudsCreateTest: public LoudsTestBase {

    typedef LoudsCreateTest                                                     MyType;

public:

    LoudsCreateTest(): LoudsTestBase("Create")
    {
        MEMORIA_ADD_TEST(runCreateRandomTest);
    }

    virtual ~LoudsCreateTest() throw () {}



    void runCreateRandomTest()
    {
    	for (Int c = 1; c <= 10; c++)
    	{
    		Allocator allocator;

    		Ctr ctr(&allocator);

    		BigInt t0 = getTimeInMillis();

    		BigInt count0 = createRandomLouds(ctr, 1000 * c, 10);

    		BigInt t1 = getTimeInMillis();

    		BigInt count1 = 0;

    		traverseTree(ctr, 0, 0, count1);

    		BigInt t2 = getTimeInMillis();

    		AssertEQ(MA_SRC, count0, count1);

    		out()<<"TreeSize: "<<count0<<" Tree Build Time: "<<FormatTime(t1-t0)<<", Traverse Time: "<<FormatTime(t2-t1)<<endl;
    	}
    }


};

}

#endif
