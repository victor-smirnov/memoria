
// Copyright 2022 Victor Smirnov
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

#include "../prototype/bt/bt_test_base.hpp"

#include <memoria/core/tools/random.hpp>

#include "store_tools.hpp"

#include <vector>

namespace memoria {
namespace tests {

class SWMRStoreTest: public TestState
{
    using MyType = SWMRStoreTest;

    using Base   = TestState;

    using Base::out;
    using Base::getRandom;

public:
    SWMRStoreTest()
    {
    }

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(suite, testLite);
    }

    void testLite()
    {
        auto wd = Base::working_directory_;
        wd.append("file.mma2");

        U8String file = wd.std_string();

        auto store = std::make_shared<LiteSWMRStoreOperation>(file, 1024);

        using StorePtrT = AllocSharedPtr<ISWMRStore<CoreApiProfile>>;
        StoreTestBench<StorePtrT> bench(store);

        bench.set_entries(1000000);
        bench.set_check_epocs(false);
        bench.set_consistency_point(ConsistencyPoint::AUTO);

        bench.run_insertions();
        bench.run_queries();
    }


};


}}
