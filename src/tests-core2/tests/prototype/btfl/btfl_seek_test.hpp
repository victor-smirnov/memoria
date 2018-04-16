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
namespace v1 {
namespace tests {

template <
    typename CtrName,
    typename AllocatorT     = PersistentInMemAllocator<>,
    typename ProfileT       = DefaultProfile<>
>
class BTFLSeekTest: public BTFLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTFLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTFLSeekTest<CtrName, AllocatorT, ProfileT>;

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

    BTFLSeekTest(String name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testSeek1);
        MEMORIA_ADD_TEST(testSeek2);
        MEMORIA_ADD_TEST(testSeek3);
        MEMORIA_ADD_TEST(testSeek4);
        MEMORIA_ADD_TEST(testSeek5);
    }

    void testSeek1()
    {
        testSeek(sampleTreeShape(10, 10, size));
    }

    void testSeek2()
    {
        testSeek(sampleTreeShape(100, 10, size));
    }

    void testSeek3()
    {
        testSeek(sampleTreeShape(10, 100, size));
    }

    void testSeek4()
    {
        testSeek(sampleTreeShape(10, 1000, size));
    }

    void testSeek5()
    {
        testSeek(sampleTreeShape(10, 10000, size));
    }



    void testSeek(const DataSizesT& shape)
    {
        out() << "Test Creation for shape: " << shape << endl;

        auto snp = branch();

        auto ctr_name = create<CtrName>(snp)->name();
        auto ctr 			= find<CtrName>(snp, ctr_name);

        auto data = fillCtrRandomly(ctr, shape);

        for (size_t c = 0; c < data.size(); c++)
        {
        		auto ii = ctr->seekL0(c);

        		AssertEQ(MA_SRC, ii->rank(0), c);
        }

        commit();
    }
};

}}}
