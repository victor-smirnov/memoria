// Copyright 2016 Victor Smirnov
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

#include <memoria/memoria.hpp>

#include "btfl_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace tests {

template <
    typename CtrName,
    typename AllocatorT     = InMemAllocator<>,
    typename ProfileT       = DefaultProfile<>
>
class BTFLRemoveTest: public BTFLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTFLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTFLRemoveTest<CtrName, AllocatorT, ProfileT>;

    using Allocator     = typename Base::Allocator;
    using AllocatorPtr  = typename Base::AllocatorPtr;
    using Ctr           = typename Base::Ctr;

    using CtrSizesT      = typename Ctr::Types::Position;

    template <int32_t Level>
    using BTFLSampleData = typename Ctr::Types::template IOData<Level>;


    using typename Base::DataSizesT;

    using Base::Streams;
    using Base::DataStreams;
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
    using Base::getRandom;


    using Base::createSampleBTFLData;
    using Base::sampleTreeShape;
    using Base::dataLength;
    using Base::checkEquality;
    using Base::fillCtrRandomly;


    using Base::dump;
    using Base::size;
    using Base::iterations;
    using Base::level_limit;
    using Base::last_level_limit;


public:

    BTFLRemoveTest()
    {}


    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testRemoval1, testRemoval2, testRemoval3);
        MMA_CLASS_TESTS(suite, testRemoval4, testRemoval5);
    }

    void testRemoval1()
    {
        testRemoval(sampleTreeShape(10, 10, size));
    }

    void testRemoval2()
    {
        testRemoval(sampleTreeShape(100, 10, size));
    }

    void testRemoval3()
    {
        testRemoval(sampleTreeShape(10, 100, size));
    }

    void testRemoval4()
    {
        testRemoval(sampleTreeShape(10, 1000, size));
    }

    void testRemoval5()
    {
        testRemoval(sampleTreeShape(10, 10000, size));
    }

    template <typename Ctr>
    auto readAllData(Ctr&& ctr)
    {
        return ctr.begin().read_entries0();
    }

    void testRemoval(const DataSizesT& shape)
    {
        out() << "Test Removal for shape: " << shape << std::endl;

        auto snp = branch();

        auto ctr_name = create<CtrName>(snp).name();
        auto ctr = find<CtrName>(snp, ctr_name);

        auto data = fillCtrRandomly(ctr, shape);

        auto ii = ctr.begin();

        auto size = data.size();

        for (size_t c = 0; c < size; c++)
        {
            size_t pos = getRandom(data.size());

            out() << "Remove element " << pos << ", size = " << data.size() << " of " << size << std::endl;

            auto ii = ctr.seekL0(pos);

            ii.remove_ge(1);

            data.erase(data.begin() + pos, data.begin() + pos + 1);

            if (c % 100 == 0) {
                auto new_data = readAllData(ctr);

                out() << "Checking container" << std::endl;
                checkEquality(data, new_data);
            }
        }

        commit();
    }
};

}}
