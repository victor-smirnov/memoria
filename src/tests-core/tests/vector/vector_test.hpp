
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

#include <memoria/v1/memoria.hpp>
#include <memoria/v1/tools/tests.hpp>

#include <memoria/v1/containers/vector/vctr_factory.hpp>

#include "../prototype/btss/btss_test_base.hpp"



#include <vector>

namespace memoria {
namespace v1 {

using namespace std;



template <
    typename CtrName,
    typename AllocatorT     = PersistentInMemAllocator<>,
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

    using BranchNodeEntry = typename Ctr::Types::BranchNodeEntry;

    using Value = typename Ctr::Types::Value;

    BigInt size = 1024*1024;

    using Base::allocator;
    using Base::snapshot;
    using Base::branch;
    using Base::commit;
    using Base::drop;
    using Base::out;
    using Base::getRandom;

public:
    VectorTest(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST_PARAM(size);

        MEMORIA_ADD_TEST_WITH_REPLAY(testCreate, replayCreate);
    }

    virtual MemBuffer createRandomBuffer(Int size)
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

        BTSSTestInputProvider<Ctr, MemBuffer> provider(data);
        ctr->begin()->insert_iobuffer(&provider);

        AssertEQ(MA_SRC, ctr->size(), data.size());

        std::vector<Value> data2(size);

        auto read = ctr->begin()->read(data2.begin(), data2.size());

        AssertEQ(MA_SRC, read, data2.size());

        for (size_t c = 0; c< data.size(); c++)
        {
            AssertEQ(MA_SRC, data[c], data2[c]);
        }

        commit();
    }

    void replayCreate() {

    }
};



}}