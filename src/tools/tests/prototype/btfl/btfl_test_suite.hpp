
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


#include "../../tests_inc.hpp"

#include "btfl_create_test.hpp"
#include "btfl_seek_test.hpp"
#include "btfl_iterator_test.hpp"
//#include "btfl_insertion_test.hpp"
//#include "btfl_removal_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class BTFLTestSuite: public TestSuite {

public:

    BTFLTestSuite(): TestSuite("BT.FL")
    {
//        registerTask(new BTFLCreateTest<BTFLTestCtr<2, PackedSizeType::VARIABLE>>("Create.Vr.2"));
//        registerTask(new BTFLCreateTest<BTFLTestCtr<2, PackedSizeType::FIXED>>("Create.Fx.2"));
//
//        registerTask(new BTFLCreateTest<BTFLTestCtr<3, PackedSizeType::VARIABLE>>("Create.Vr.3"));
//        registerTask(new BTFLCreateTest<BTFLTestCtr<3, PackedSizeType::FIXED>>("Create.Fx.3"));
//

//        registerTask(new BTFLCreateTest<BTFLTestCtr<4>>("Create.4"));
//        registerTask(new BTFLSeekTest<BTFLTestCtr<4>>("Seek.4"));
    	registerTask(new BTFLIteratorTest<BTFLTestCtr<4>>("Iterator.4"));
//
//
//        registerTask(new BTFLIterTest<BTFLTestCtr<2, PackedSizeType::VARIABLE>>("Iter.Vr.2"));
//        registerTask(new BTFLIterTest<BTFLTestCtr<2, PackedSizeType::FIXED>>("Iter.Fx.2"));
//
//        registerTask(new BTFLIterTest<BTFLTestCtr<3, PackedSizeType::VARIABLE>>("Iter.Vr.3"));
//        registerTask(new BTFLIterTest<BTFLTestCtr<3, PackedSizeType::FIXED>>("Iter.Fx.3"));
//
//        registerTask(new BTFLIterTest<BTFLTestCtr<4, PackedSizeType::VARIABLE>>("Iter.Vr.4"));
//        registerTask(new BTFLIterTest<BTFLTestCtr<4, PackedSizeType::FIXED>>("Iter.Fx.4"));
//
//        registerTask(new BTFLInsertionTest<BTFLTestCtr<2, PackedSizeType::FIXED>>("Insert.Fx.2"));
//        registerTask(new BTFLInsertionTest<BTFLTestCtr<2, PackedSizeType::VARIABLE>>("Insert.Vr.2"));
//
//        registerTask(new BTFLInsertionTest<BTFLTestCtr<3, PackedSizeType::FIXED>>("Insert.Fx.3"));
//        registerTask(new BTFLInsertionTest<BTFLTestCtr<3, PackedSizeType::VARIABLE>>("Insert.Vr.3"));
//
//        registerTask(new BTFLInsertionTest<BTFLTestCtr<4, PackedSizeType::FIXED>>("Insert.Fx.4"));
//        registerTask(new BTFLInsertionTest<BTFLTestCtr<4, PackedSizeType::VARIABLE>>("Insert.Vr.4"));
//
//        registerTask(new BTFLRemovalTest<BTFLTestCtr<2, PackedSizeType::FIXED>>("Remove.Fx.2"));
//        registerTask(new BTFLRemovalTest<BTFLTestCtr<2, PackedSizeType::VARIABLE>>("Remove.Vr.2"));
//
//        registerTask(new BTFLRemovalTest<BTFLTestCtr<3, PackedSizeType::FIXED>>("Remove.Fx.3"));
//        registerTask(new BTFLRemovalTest<BTFLTestCtr<3, PackedSizeType::VARIABLE>>("Remove.Vr.3"));
//
//        registerTask(new BTFLRemovalTest<BTFLTestCtr<4, PackedSizeType::FIXED>>("Remove.Fx.4"));
//        registerTask(new BTFLRemovalTest<BTFLTestCtr<4, PackedSizeType::VARIABLE>>("Remove.Vr.4"));
    }
};

}}
