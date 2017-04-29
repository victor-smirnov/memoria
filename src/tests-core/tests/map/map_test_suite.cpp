
// Copyright 2012 Victor Smirnov
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


#include <memoria/v1/tools/tests_inc.hpp>

#include "map_create_test.hpp"
#include "map_remove_test.hpp"

#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

class MapTestSuite: public TestSuite {

public:

    MapTestSuite(): TestSuite("MapSuite")
    {
        registerTask(new MapRemoveTest<Map<UUID, BigInt>>("MapM.Remove"));
        registerTask(new MapCreateTest<Map<UUID, BigInt>>("MapM.Create"));
    }
};


MMA1_REGISTER_TEST_SUITE(MapTestSuite)

}}
