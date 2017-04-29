
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
        registerTask(new BitmapMiscTest<UInt>("Misc.UInt"));
        registerTask(new BitmapMiscTest<UBigInt>("Misc.UBigInt"));

        registerTask(new BitmapRankTest<UBigInt>("Rank.UBigInt"));
        registerTask(new BitmapRankTest<UInt>("Rank.UInt"));

        registerTask(new BitmapCountTest<UBigInt>("Count.UBigInt"));
        registerTask(new BitmapCountTest<UInt>("Count.UInt"));

        registerTask(new BitmapSelectTest("Select"));

        registerTask(new BitmapSpeedTest<UBigInt>("Speed"));
    }

};


MMA1_REGISTER_TEST_SUITE(BitmapTestSuite)

}}