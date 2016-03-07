
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_TEST_SUITE_HPP_
#define MEMORIA_TESTS_VECTOR_TEST_SUITE_HPP_

#include <memoria/core/types/types.hpp>

#include "vector_test.hpp"

namespace memoria {

using namespace std;

class VectorTestSuite: public TestSuite {

public:

    VectorTestSuite(): TestSuite("Vector")
    {
        registerTask(new VectorTest<Vector<Int>>("Int.FX"));
        registerTask(new VectorTest<Vector<VLen<Granularity::Byte>>>("Int.VL.Byte"));
        registerTask(new VectorTest<Vector<VLen<Granularity::Bit>>>("Int.VL.Bit"));
    }
};

}


#endif

