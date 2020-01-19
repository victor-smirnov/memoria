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
class BTFLCreateTest: public BTFLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTFLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTFLCreateTest<CtrName, AllocatorT, ProfileT>;

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

    BTFLCreateTest()
    {
    }

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testCreation1, testCreation2, testCreation3);
        MMA1_CLASS_TESTS(suite, testCreation4, testCreation5);
    }



    void testCreation1()
    {
        testCreation(sampleTreeShape(10, 10, size));
    }

    void testCreation2()
    {
        testCreation(sampleTreeShape(100, 10, size));
    }

    void testCreation3()
    {
        testCreation(sampleTreeShape(10, 100, size));
    }

    void testCreation4()
    {
        testCreation(sampleTreeShape(10, 1000, size));
    }

    void testCreation5()
    {
        testCreation(sampleTreeShape(10, 10000, size));
    }



    void testCreation(const DataSizesT& shape)
    {
        out() << "Test Creation for shape: " << shape << std::endl;

        auto snp = branch();

        auto ctr_name = create<CtrName>(snp).name();
        auto ctr = find<CtrName>(snp, ctr_name);

        auto data = fillCtrRandomly(ctr, shape);
        auto data_len = dataLength(data);

        auto rdata = ctr.begin().template read_data0(data_len);

        checkEquality(data, rdata);

        auto ii = ctr.begin();

        for (size_t c = 0; c < data.size(); )
        {
            int32_t len = getRandom(5) + 1;

            auto entry = ii.read_entries0(len);
            assert_le(entry.size(), len);

            for (size_t l = 0; l < entry.size(); l++)
            {
                assert_equals(std::get<0>(data[c + l]), std::get<0>(entry[l]));
            	checkEquality(std::get<1>(data[c + l]), std::get<1>(entry[l]));
            }

            c += entry.size();
        }

        commit();
    }
};

}}
