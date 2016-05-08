
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

#include "bttl_create_test.hpp"
#include "bttl_iter_test.hpp"
#include "bttl_insertion_test.hpp"
#include "bttl_removal_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class BTTLTestSuite: public TestSuite {

public:

    BTTLTestSuite(): TestSuite("BT.TL")
    {
//        registerTask(new BTTLCreateTest<BTTLTestCtr<2, PackedSizeType::VARIABLE>>("Create.Vr.2"));
//        registerTask(new BTTLCreateTest<BTTLTestCtr<2, PackedSizeType::FIXED>>("Create.Fx.2"));
//
//        registerTask(new BTTLCreateTest<BTTLTestCtr<3, PackedSizeType::VARIABLE>>("Create.Vr.3"));
//        registerTask(new BTTLCreateTest<BTTLTestCtr<3, PackedSizeType::FIXED>>("Create.Fx.3"));
//
//        registerTask(new BTTLCreateTest<BTTLTestCtr<4, PackedSizeType::VARIABLE>>("Create.Vr.4"));
//        registerTask(new BTTLCreateTest<BTTLTestCtr<4, PackedSizeType::FIXED>>("Create.Fx.4"));
//
//
//        registerTask(new BTTLIterTest<BTTLTestCtr<2, PackedSizeType::VARIABLE>>("Iter.Vr.2"));
//        registerTask(new BTTLIterTest<BTTLTestCtr<2, PackedSizeType::FIXED>>("Iter.Fx.2"));
//
//        registerTask(new BTTLIterTest<BTTLTestCtr<3, PackedSizeType::VARIABLE>>("Iter.Vr.3"));
//        registerTask(new BTTLIterTest<BTTLTestCtr<3, PackedSizeType::FIXED>>("Iter.Fx.3"));
//
//        registerTask(new BTTLIterTest<BTTLTestCtr<4, PackedSizeType::VARIABLE>>("Iter.Vr.4"));
//        registerTask(new BTTLIterTest<BTTLTestCtr<4, PackedSizeType::FIXED>>("Iter.Fx.4"));
//
//        registerTask(new BTTLInsertionTest<BTTLTestCtr<2, PackedSizeType::FIXED>>("Insert.Fx.2"));
//        registerTask(new BTTLInsertionTest<BTTLTestCtr<2, PackedSizeType::VARIABLE>>("Insert.Vr.2"));
//
//        registerTask(new BTTLInsertionTest<BTTLTestCtr<3, PackedSizeType::FIXED>>("Insert.Fx.3"));
//        registerTask(new BTTLInsertionTest<BTTLTestCtr<3, PackedSizeType::VARIABLE>>("Insert.Vr.3"));
//
        registerTask(new BTTLInsertionTest<BTTLTestCtr<4, PackedSizeType::FIXED>>("Insert.Fx.4"));
        registerTask(new BTTLInsertionTest<BTTLTestCtr<4, PackedSizeType::VARIABLE>>("Insert.Vr.4"));
//
//        registerTask(new BTTLRemovalTest<BTTLTestCtr<2, PackedSizeType::FIXED>>("Remove.Fx.2"));
//        registerTask(new BTTLRemovalTest<BTTLTestCtr<2, PackedSizeType::VARIABLE>>("Remove.Vr.2"));
//
//        registerTask(new BTTLRemovalTest<BTTLTestCtr<3, PackedSizeType::FIXED>>("Remove.Fx.3"));
//        registerTask(new BTTLRemovalTest<BTTLTestCtr<3, PackedSizeType::VARIABLE>>("Remove.Vr.3"));
//
//        registerTask(new BTTLRemovalTest<BTTLTestCtr<4, PackedSizeType::FIXED>>("Remove.Fx.4"));
//        registerTask(new BTTLRemovalTest<BTTLTestCtr<4, PackedSizeType::VARIABLE>>("Remove.Vr.4"));
    }
};

}}
