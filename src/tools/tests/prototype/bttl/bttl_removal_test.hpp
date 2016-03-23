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
class BTTLRemovalTest;

template <
    Int Levels,
    PackedSizeType SizeType,
    typename AllocatorT,
    typename ProfileT
>
class BTTLRemovalTest<BTTLTestCtr<Levels, SizeType>, AllocatorT, ProfileT>: public BTTLTestBase<BTTLTestCtr<Levels, SizeType>, AllocatorT, ProfileT> {

    using CtrName = BTTLTestCtr<Levels, SizeType>;

    using Base   = BTTLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTTLRemovalTest<CtrName, AllocatorT, ProfileT>;

    using Allocator     = typename Base::Allocator;
    using AllocatorPtr  = typename Base::AllocatorPtr;
    using Ctr           = typename Base::Ctr;

    using DetInputProvider      = bttl::DeterministicDataInputProvider<Ctr>;
    using RngInputProvider      = bttl::RandomDataInputProvider<Ctr, RngInt>;

    using Rng            = typename RngInputProvider::Rng;

    using CtrSizesT      = typename Ctr::Types::Position;
    using CtrSizeT       = typename Ctr::Types::CtrSizeT;

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
    using Base::checkSoftMemLimit;
    using Base::getRandom;

    using Base::fillCtr;
    using Base::checkRanks;
    using Base::checkExtents;
    using Base::checkSubtree;
    using Base::checkTree;
    using Base::sampleTreeShape;
    using Base::checkIterator;
    using Base::getIntTestGenerator;

    using Base::dump;
    using Base::size;
    using Base::iterations;
    using Base::level_limit;
    using Base::last_level_limit;


    Int level  = -1;

    CtrSizeT    length_;
    CtrSizesT   removal_pos_;
    UUID        ctr_name_;

    Int level_;

public:

    BTTLRemovalTest(String name):
        Base(name)
    {

        MEMORIA_ADD_TEST_PARAM(level);

        MEMORIA_ADD_TEST_PARAM(length_)->state();
        MEMORIA_ADD_TEST_PARAM(removal_pos_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(level_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(testRemove, replayRemove);
    }

    virtual ~BTTLRemovalTest() throw () {}

    virtual void smokeCoverage(Int scale)
    {
        this->size          = 10000  * scale;
        this->iterations    = 1;
    }

    virtual void smallCoverage(Int scale)
    {
        this->size          = 100000  * scale;
        this->iterations    = 3;
    }

    virtual void normalCoverage(Int scale)
    {
        this->size          = 100000 * scale;
        this->iterations    = 50;
    }

    virtual void largeCoverage(Int scale)
    {
        this->size          = 1000000 * scale;
        this->iterations    = 10;
    }


    void testRemovalStep(Ctr& ctr)
    {
        auto iter = ctr.seek(removal_pos_[0]);

        for (Int s = 1; s <= level_; s++) {
            iter->toData(removal_pos_[s]);
        }

        auto sizes_before = ctr.sizes();

        out()<<"Remove "<<length_<<" elements data at: "<<removal_pos_<<", Ctr size before data removal: "<<sizes_before<<endl;

        iter->remove_subtrees(length_);

        checkIterator(MA_RAW_SRC, iter, removal_pos_, level_);

        auto sizes_after = ctr.sizes();
        auto ctr_totals = ctr.total_counts();

        out()<<"Totals: "<<ctr_totals<<" "<<sizes_after<<endl;
        AssertEQ(MA_SRC, ctr_totals, sizes_after);

        check(MA_SRC, "Remove: Container Check Failed");

        checkExtents(ctr);

        checkTree(ctr);
    }

    void replayRemove()
    {
        out()<<"Replay!"<<endl;

        auto snp = branch();

        auto ctr = find<CtrName>(snp, ctr_name_);

        testRemovalStep(*ctr.get());

        commit();
    }

    CtrSizeT sampleSize(Int iteration, CtrSizeT size)
    {
        if (iteration % 3 == 0)
        {
            return this->getRandom(size);
        }
        else if (iteration % 3 == 1) {
            return size - 1;
        }
        else {
            return 0;
        }
    }

    void testRemove() {

        if (level == -1)
        {
            for (Int c = 0; c < Levels; c++)
            {
                testRemovalForLevel(c);
                out()<<endl;
            }
        }
        else {
            testRemovalForLevel(level);
        }
    }

    void testRemovalForLevel(Int level)
    {
        out()<<"Test for level: "<<level<<endl;

        auto snp = branch();

        auto ctr_name = create<CtrName>(snp)->name();

        commit();

        auto shape = sampleTreeShape();

        BigInt total_sum;

        {
            auto snp = branch();
            auto ctr = find<CtrName>(snp, ctr_name);

            out()<<"shape: "<<shape<<endl;

            RngInputProvider provider(shape, this->getIntTestGenerator());

            fillCtr(*ctr.get(), provider);

            total_sum = ctr->sizes().sum();

            commit();
        }


        for (Int c = 0; c < iterations && total_sum > 0; c++)
        {
            out()<<"Iteration: "<<c<<endl;

            auto snp = branch();
            auto ctr = find<CtrName>(snp, ctr_name);

            auto sizes = ctr->sizes();

            removal_pos_ = CtrSizesT(-1);

            removal_pos_[0] = sampleSize(c, sizes[0]);

            auto iter = ctr->seek(removal_pos_[0]);
            level_ = 0;

            for (Int s = 1; s <= level; s++)
            {
                auto local_size = iter->substream_size();

                if (local_size > 0)
                {
                    removal_pos_[s] = sampleSize(c, local_size);

                    iter->toData(removal_pos_[s]);
                    level_ = s;
                }
                else {
                    break;
                }
            }

            auto pos  = iter->pos();
            auto size = iter->size();

            auto len = size - pos;

            if (len > 0) {
                length_ = getRandom(len - 1) + 1;
            }
            else {
                length_ = 0;
            }

            testRemovalStep(*ctr.get());

            out()<<"Sizes: "<<ctr->sizes()<<endl<<endl;

            total_sum = ctr->sizes().sum();

            commit();
        }
    }
};

}}