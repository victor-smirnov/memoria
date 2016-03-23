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
class BTTLIterTest: public BTTLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTTLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTTLIterTest<CtrName, AllocatorT, ProfileT>;

    using Allocator     = typename Base::Allocator;
    using AllocatorPtr  = typename Base::AllocatorPtr;
    using Ctr           = typename Base::Ctr;

    using DetInputProvider      = bttl::DeterministicDataInputProvider<Ctr>;
    using RngInputProvider      = bttl::RandomDataInputProvider<Ctr, RngInt>;

    using Rng            = typename RngInputProvider::Rng;

    using CtrSizeT       = typename Ctr::Types::CtrSizeT;
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
    using Base::getIntTestGenerator;
    using Base::checkIterator;
    using Base::scanAndCheckIterator;


    using Base::dump;
    using Base::size;
    using Base::iterations;
    using Base::level_limit;
    using Base::last_level_limit;

    struct ScanFn {
        Byte expected_;

        ScanFn(Byte expected): expected_(expected) {}

        template <typename Stream>
        void operator()(const Stream* obj, Int start, Int end)
        {
            for (Int c = start; c < end; c++)
            {
                MEMORIA_V1_ASSERT(obj->value(c), ==, expected_);
            }
        }
    };

public:

    BTTLIterTest(String name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testDetProvider);
        MEMORIA_ADD_TEST(testRngProvider);
    }

    virtual ~BTTLIterTest() throw () {}


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

            auto totals = fillCtr(*ctr.get(), provider);

            AssertEQ(MA_RAW_SRC, totals, ctr->sizes());

            check(snp, "Snapshot check failed", MA_RAW_SRC);

            scanAndCheckIterator(*ctr.get());

            for (CtrSizeT r = 0; r < totals[0]; r++)
            {
                CtrSizesT path(-1);
                path[0] = r;

                checkSubtree(*ctr.get(), path);
            }

            out()<<endl;
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

            snp = branch();
            auto ctr = find<CtrName>(snp, ctr_name);

            auto shape = sampleTreeShape();

            out()<<"shape: "<<shape<<endl;

            RngInputProvider provider(shape, getIntTestGenerator());

            auto totals = fillCtr(*ctr.get(), provider);

            check(snp, "Snapshot check failed", MA_RAW_SRC);

            scanAndCheckIterator(*ctr.get());

            for (CtrSizeT r = 0; r < totals[0]; r++)
            {
                CtrSizesT path(-1);
                path[0] = r;

                checkSubtree(*ctr.get(), path);
            }

            out()<<endl;
        }
    }


//    void checkScan(Ctr& ctr, CtrSizesT sizes)
//    {
//      auto i = ctr.seek(0);
//
//      CtrSizesT extent;
//
//      long t2 = getTimeInMillis();
//
//      auto rows = sizes[0];
//      auto cols = sizes[1];
//
//      auto iter = ctr.seek(0);
//      for (Int r = 0; r < rows; r++)
//      {
//          AssertEQ(MA_SRC, iter.pos(), r);
//          AssertEQ(MA_SRC, iter.cache().abs_pos()[0], r);
//          AssertEQ(MA_SRC, iter.size(), rows);
//
//          iter.toData();
//          iter.checkPrefix();
//
//          for (Int c = 0; c < cols; c++)
//          {
//              AssertEQ(MA_SRC, iter.pos(), c);
//              AssertEQ(MA_SRC, iter.size(), cols);
//
//              iter.toData();
//
//              ScanFn scan_fn((c + 1) % 256);
//              auto scanned = iter.template scan<IntList<2>>(scan_fn);
//
//              AssertEQ(MA_SRC, scanned, iter.size());
//
//              AssertTrue(MA_SRC, iter.isSEnd());
//
//              iter.toIndex();
//              iter.skipFw(1);
//          }
//
//
//          iter.toIndex();
//
//          iter.skipFw(1);
//      }
//
//      long t3 = getTimeInMillis();
//
//      this->out()<<"Scan time: "<<FormatTime(t3 - t2)<<endl;
//    }
};

}}