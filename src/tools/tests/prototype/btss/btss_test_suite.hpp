
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include "../../tests_inc.hpp"
#include "btss_batch_test.hpp"

#include "btss_core_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class BTSSTestSuite: public TestSuite {

public:

    BTSSTestSuite(): TestSuite("BT.SS")
    {
        registerTask(new BTSSBatchTest<BTSSTestCtr<PackedSizeType::FIXED, PackedSizeType::FIXED>>("Batch.FX.FX"));
        registerTask(new BTSSBatchTest<BTSSTestCtr<PackedSizeType::FIXED, PackedSizeType::VARIABLE>>("Batch.FX.VL"));
        registerTask(new BTSSBatchTest<BTSSTestCtr<PackedSizeType::VARIABLE, PackedSizeType::FIXED>>("Batch.VL.FX"));
        registerTask(new BTSSBatchTest<BTSSTestCtr<PackedSizeType::VARIABLE, PackedSizeType::VARIABLE>>("Batch.VL.VL"));
    }
};

}}