
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


#include <memoria/v1/tests/tests_inc.hpp>

#include "multimap_basic_test.hpp"

#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

class MultiMapTestSuite: public TestSuite {

public:

    MultiMapTestSuite(): TestSuite("MMapSuite")
    {
        //registerTask(new MultiMapBasicTest<Multimap<String, int64_t>>("Basic.S.I"));
        registerTask(new MultiMapBasicTest<Multimap<double, int64_t>>("Basic.D.I"));
        registerTask(new MultiMapBasicTest<Multimap<int64_t, int64_t>>("Basic.I.I"));
        registerTask(new MultiMapBasicTest<Multimap<UUID, int64_t>>("Basic.U.I"));
        //registerTask(new MultiMapBasicTest<Multimap<UUID, String>>("Basic.U.S"));
    }

};

MMA1_REGISTER_TEST_SUITE(MultiMapTestSuite)



}}
