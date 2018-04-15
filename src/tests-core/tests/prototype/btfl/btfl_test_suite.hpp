
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

#include "btfl_create_test.hpp"
#include "btfl_seek_test.hpp"
#include "btfl_iterator_test.hpp"
#include "btfl_removal_test.hpp"

namespace memoria {
namespace v1 {

class BTFLTestSuite: public TestSuite {

public:

    BTFLTestSuite(): TestSuite("BT.FL")
    {
      registerTask(new BTFLCreateTest<BTFLTestCtr<2>>("Create.2"));
      registerTask(new BTFLSeekTest<BTFLTestCtr<2>>("Seek.2"));
      registerTask(new BTFLIteratorTest<BTFLTestCtr<2>>("Iterator.2"));
      registerTask(new BTFLRemoveTest<BTFLTestCtr<2>>("Remove.2"));

      registerTask(new BTFLCreateTest<BTFLTestCtr<4>>("Create.4"));
      registerTask(new BTFLSeekTest<BTFLTestCtr<4>>("Seek.4"));
      registerTask(new BTFLIteratorTest<BTFLTestCtr<4>>("Iterator.4"));
      registerTask(new BTFLRemoveTest<BTFLTestCtr<4>>("Remove.4"));
    }
};

}}
