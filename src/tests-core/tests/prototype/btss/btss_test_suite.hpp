
// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once


#include <memoria/v1/tools/tests_inc.hpp>

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