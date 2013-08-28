
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BITMAP_BITMAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_BITMAP_BITMAP_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "bitmap_misc_test.hpp"
#include "bitmap_rank_test.hpp"
#include "bitmap_count_test.hpp"
#include "bitmap_select_test.hpp"
#include "bitmap_speed_test.hpp"


#include <vector>

namespace memoria {

using namespace memoria::vapi;
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

}


#endif

