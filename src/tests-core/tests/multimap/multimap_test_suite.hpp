
// Copyright 2016 Victor Smirnov
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

#include "multimap_create_test.hpp"
//#include "multimap_remove_test.hpp"

#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

class MultiMapTestSuite: public TestSuite {

public:

    MultiMapTestSuite(): TestSuite("MMapSuite")
    {
//        registerTask(new MultiMapCreateTest<Map<String, Vector<int64_t>>>("Create.S.I"));
//        registerTask(new MultiMapCreateTest<Map<double, Vector<int64_t>>>("Create.D.I"));
        registerTask(new MultiMapCreateTest<Map<int64_t, Vector<int64_t>>>("Create.I.I"));
//        registerTask(new MultiMapCreateTest<Map<int64_t, Vector<int64_t>>>("Create.I.S"));
//        registerTask(new MultiMapCreateTest<Map<UUID,   Vector<int64_t>>>("Create.U.I"));
//
//        registerTask(new MultiMapRemoveTest<Map<String, Vector<int64_t>>>("Remove.S.I"));
//        registerTask(new MultiMapRemoveTest<Map<double, Vector<int64_t>>>("Remove.D.I"));
//        registerTask(new MultiMapRemoveTest<Map<int64_t, Vector<int64_t>>>("Remove.I.I"));
//        registerTask(new MultiMapRemoveTest<Map<int64_t, Vector<int64_t>>>("Remove.I.S"));
//        registerTask(new MultiMapRemoveTest<Map<UUID,   Vector<int64_t>>>("Remove.U.I"));
    }

};

}}
