
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


#pragma once

#include "../prototype/bt/bt_test_base.hpp"

#include <memoria/api/vector/vector_api.hpp>
#include <memoria/core/tools/random.hpp>

#include <vector>

namespace memoria {
namespace tests {




template <
    typename DataType,
    typename ProfileT = NoCowProfile<>,
    typename StoreT   = IMemoryStorePtr<ProfileT>
>
class VectorTest: public BTTestBase<Vector<DataType>, ProfileT, StoreT>
{
    using MyType = VectorTest;

    using Base   = BTTestBase<Vector<DataType>, ProfileT, StoreT>;

    using typename Base::CtrApi;

    using typename Base::Store;
    using typename Base::StorePtr;

    using CxxElementViewType = DTTViewType<DataType>;

    int64_t size = 1024*1024;

    using Base::store;
    using Base::snapshot;
    using Base::branch;
    using Base::commit;
    using Base::drop;
    using Base::out;
    using Base::getRandom;

public:
    VectorTest()
    {
    }

    MMA_STATE_FILEDS(size)

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TEST_WITH_REPLAY(suite, testCreate, replayCreate);
    }




    void testCreate()
    {
        auto snp = branch();

        auto ctr = create<Vector<DataType>>(snp, Vector<DataType>{});

        commit();
    }

    void replayCreate() {

    }
};


}}
