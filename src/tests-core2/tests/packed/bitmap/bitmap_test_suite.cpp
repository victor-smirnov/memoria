
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include "bitmap_misc_test.hpp"
#include "bitmap_rank_test.hpp"
#include "bitmap_count_test.hpp"
#include "bitmap_select_test.hpp"
#include "bitmap_speed_test.hpp"


namespace memoria {
namespace v1 {
namespace tests {

namespace {

MMA1_BITMAP_MISC_SUITE(BitmapMiscSuite32, uint32_t);
//MMA1_BITMAP_MISC_SUITE(BitmapMiscSuite64, uint64_t);

//MMA1_BITMAP_RANK_SUITE(BitmapRankSuite32, uint32_t);
//MMA1_BITMAP_RANK_SUITE(BitmapRankSuite64, uint64_t);

//MMA1_BITMAP_COUNT_SUITE(BitmapCountSuite32, uint32_t);
//MMA1_BITMAP_COUNT_SUITE(BitmapCountSuite64, uint64_t);

//MMA1_BITMAP_SELECT_SUITE();

//MMA1_BITMAP_SPEED_SUITE(BitmapSpeedSuite64, uint64_t);

}


}}}
