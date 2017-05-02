
// Copyright 2013 Victor Smirnov
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

#include "bitmap_misc_test.hpp"
#include "bitmap_rank_test.hpp"
#include "bitmap_count_test.hpp"
#include "bitmap_select_test.hpp"
#include "bitmap_speed_test.hpp"


#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

class BitmapTestSuite: public TestSuite {

public:

    BitmapTestSuite(): TestSuite("BitmapSuite")
    {
        registerTask(new BitmapMiscTest<uint32_t>("Misc.uint32_t"));
        registerTask(new BitmapMiscTest<uint64_t>("Misc.uint64_t"));

        registerTask(new BitmapRankTest<uint64_t>("Rank.uint64_t"));
        registerTask(new BitmapRankTest<uint32_t>("Rank.uint32_t"));

        registerTask(new BitmapCountTest<uint64_t>("Count.uint64_t"));
        registerTask(new BitmapCountTest<uint32_t>("Count.uint32_t"));

        registerTask(new BitmapSelectTest("Select"));

        registerTask(new BitmapSpeedTest<uint64_t>("Speed"));
    }

};


MMA1_REGISTER_TEST_SUITE(BitmapTestSuite)

}}
