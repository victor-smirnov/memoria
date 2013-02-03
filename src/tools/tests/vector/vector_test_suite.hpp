
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_VECTOR_TEST_SUITE_HPP_
#define MEMORIA_TESTS_VECTOR_VECTOR_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "vector_test.hpp"
#include "vector_transfer_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class VectorTestSuite: public TestSuite {

public:

    VectorTestSuite(): TestSuite("VectorSuite")
    {
//        registerTask(new VectorTest<UByte>("UByte"));
        registerTask(new VectorTest<Int>("Int"));
//        registerTask(new VectorTest<BigInt>("BigInt"));

//        registerTask(new VectorTransferTest<UByte>("Transfer"));
    }

};

}


#endif

