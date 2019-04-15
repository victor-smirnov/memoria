
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

#include "../prototype/btss/btss_test_base.hpp"

#include <memoria/v1/api/vector/vector_api.hpp>

#include <vector>

namespace memoria {
namespace v1 {
namespace tests {


template <
    typename CtrName,
    typename AllocatorT     = InMemAllocator<>,
    typename ProfileT       = DefaultProfile<>
>
class VectorTest: public BTSSTestBase<CtrName, AllocatorT, ProfileT>
{
    using MyType = VectorTest<CtrName, AllocatorT, ProfileT>;

    using Base = BTSSTestBase<CtrName, AllocatorT, ProfileT>;

    using typename Base::Ctr;
    using typename Base::Iterator;
    using typename Base::Allocator;
    using typename Base::AllocatorPtr;
    using typename Base::MemBuffer;
    using typename Base::Entry;

    //using BranchNodeEntry = typename Ctr::Types::BranchNodeEntry;

    using Value = typename Ctr::DataValue;

    int64_t size = 1024*1024;

    using Base::allocator;
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

    MMA1_STATE_FILEDS(size)

    static void init_suite(TestSuite& suite) {
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testCreate, replayCreate);
    }


    virtual MemBuffer createRandomBuffer(int32_t size)
    {
        auto buffer = MemBuffer(size);

        for (auto& v: buffer)
        {
            v = getRandom(100);
        }

        return buffer;
    }


    void testCreate()
    {
        auto snp = branch();

        auto ctr = create<CtrName>(snp);

        std::vector<Value> data(size);

        for (auto& d: data)
        {
            d = this->getRandom(100);
        }
#ifdef MMA1_USE_IOBUFFER
        ctr.begin().insert(data);
#endif
        assert_equals(ctr.size(), data.size());

        std::vector<Value> data2 = ctr.begin().read(size);

        assert_equals(size, data2.size());

        for (size_t c = 0; c< data.size(); c++)
        {
            assert_equals(data[c], data2[c]);
        }

        commit();
    }

    void replayCreate() {

    }
};



}}}
