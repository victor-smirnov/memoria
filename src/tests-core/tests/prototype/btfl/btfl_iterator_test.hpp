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
class BTFLIteratorTest: public BTFLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTFLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTFLIteratorTest<CtrName, AllocatorT, ProfileT>;

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
    BTFLIteratorTest()
    {}

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testIterator1, testIterator2, testIterator3);
        MMA1_CLASS_TESTS(suite, testIterator4, testIterator5, testIterator6);
    }

    void testIterator1()
    {
    	testIterator(sampleTreeShape(10, 10, size));
    }

    void testIterator2()
    {
        testIterator(sampleTreeShape(100, 10, size));
    }

    void testIterator3()
    {
        testIterator(sampleTreeShape(10, 100, size));
    }

    void testIterator4()
    {
        testIterator(sampleTreeShape(10, 1000, size));
    }

    void testIterator5()
    {
        testIterator(sampleTreeShape(10, 10000, size));
    }

    void testIterator6()
    {
        testIterator(sampleTreeShape(10, 100000, size));
    }



    void testIterator(const DataSizesT& shape)
    {
        out() << "Test Iteration for shape: " << shape << std::endl;

        auto snp = branch();

        auto ctr_name = create<CtrName>(snp).name();
        auto ctr 			= find<CtrName>(snp, ctr_name);

        auto data = fillCtrRandomly(ctr, shape);

        btfl_test::BTFLDataChecker<decltype(data), DataStreams, 0> checker(data);

        auto ii = ctr.begin();

        checker.check(ii);

        commit();
    }
};

}}
