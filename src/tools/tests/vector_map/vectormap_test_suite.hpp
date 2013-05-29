
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "vectormap_create_test.hpp"
#include "vectormap_insertdata_test.hpp"
#include "vectormap_removedata_test.hpp"
#include "vectormap_remove_test.hpp"
#include "vectormap_replace_test.hpp"

//#include "vectormap_transfer_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class VectorMapTestSuite: public TestSuite {

public:

    VectorMapTestSuite(): TestSuite("VectorMapSuite")
    {
        registerTask(new VectorMapCreateTest<BigInt, BigInt>());
        registerTask(new VectorMapInsertDataTest<BigInt, BigInt>());
        registerTask(new VectorMapRemoveDataTest<BigInt, BigInt>());
        registerTask(new VectorMapRemoveTest<BigInt, BigInt>());
        registerTask(new VectorMapReplaceTest<BigInt, BigInt>());
    }

};

}


#endif

