
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTSS_TEST_SUITE_HPP_
#define MEMORIA_TESTS_BTSS_TEST_SUITE_HPP_


#include "../../tests_inc.hpp"

#include "btss_core_test.hpp"
#include "btss_batch_insertion_test.hpp"
//#include "btss_batch_deletion_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class BTSSTestSuite: public TestSuite {

public:

    BTSSTestSuite(): TestSuite("BT.SS")
    {
//    	registerTask(new BTSSCoreTest<BTSSTestCtr<PackedSizeType::FIXED, PackedSizeType::FIXED>>("Core"));
//    	registerTask(new BTSSBatchInsertionTest<BTSSTestCtr<PackedSizeType::FIXED, PackedSizeType::FIXED>>("Batch.FX.FX"));
    	registerTask(new BTSSBatchInsertionTest<BTSSTestCtr<PackedSizeType::FIXED, PackedSizeType::VARIABLE>>("Batch.FX.VL"));
//    	registerTask(new BTSSBatchInsertionTest<BTSSTestCtr<PackedSizeType::VARIABLE, PackedSizeType::FIXED>>("Batch.VL.FX"));
//    	registerTask(new BTSSBatchInsertionTest<BTSSTestCtr<PackedSizeType::VARIABLE, PackedSizeType::VARIABLE>>("Batch.VL.VL"));
    }
};

}


#endif

