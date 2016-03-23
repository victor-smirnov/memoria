// Copyright 2015 Victor Smirnov
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
#include <memoria/v1/tools/tools.hpp>

#include "bttl_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace v1 {

template <
    typename CtrName,
    typename AllocatorT     = PersistentInMemAllocator<>,
    typename ProfileT       = DefaultProfile<>
>
class BTTLCreateTest: public BTTLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTTLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTTLCreateTest<CtrName, AllocatorT, ProfileT>;

    using Allocator     = typename Base::Allocator;
    using AllocatorPtr  = typename Base::AllocatorPtr;
    using Ctr           = typename Base::Ctr;

    using DetInputProvider      = bttl::DeterministicDataInputProvider<Ctr>;
    using RngInputProvider      = bttl::RandomDataInputProvider<Ctr, RngInt>;

    using Rng            = typename RngInputProvider::Rng;

    using CtrSizesT      = typename Ctr::Types::Position;

    static const Int Streams = Ctr::Types::Streams;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;

    using Base::fillCtr;
    using Base::checkRanks;
    using Base::checkExtents;
    using Base::checkSubtree;
    using Base::sampleTreeShape;

    using Base::dump;
    using Base::size;
    using Base::iterations;
    using Base::level_limit;
    using Base::last_level_limit;


public:

    BTTLCreateTest(String name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testDetProvider);
        MEMORIA_ADD_TEST(testRngProvider);
    }

    virtual ~BTTLCreateTest() throw () {}



    void testDetProvider()
    {
        auto snp = branch();

        auto ctr_name = create<CtrName>(snp)->name();

        commit();

        for (Int i = 0; i < iterations; i++)
        {
            out()<<"Iteration "<<(i + 1)<<endl;

            snp = branch();

            auto ctr = find<CtrName>(snp, ctr_name);
            auto shape = sampleTreeShape();

            out()<<"shape: "<<shape<<endl;

            DetInputProvider provider(shape);

            testProvider(snp, *ctr.get(), provider);

            commit();
        }
    }

    void testRngProvider()
    {
        auto snp = branch();

        auto ctr_name = create<CtrName>(snp)->name();

        commit();

        for (Int i = 0; i < iterations; i++)
        {
            out()<<"Iteration "<<(i + 1)<<endl;

            auto snp = branch();

            auto ctr = find<CtrName>(snp, ctr_name);

            auto shape = this->sampleTreeShape();

            out()<<"shape: "<<shape<<endl;

            RngInputProvider provider(shape, this->getIntTestGenerator());

            testProvider(snp, *ctr.get(), provider);

            commit();
        }
    }


    template <typename Snapshot, typename Provider>
    void testProvider(Snapshot&& snp, Ctr& ctr, Provider& provider)
    {
        fillCtr(ctr, provider);

        check(snp, "Snapshot check failed", MA_RAW_SRC);

        checkRanks(ctr);

        out()<<endl;
    }
};

}}