
// Copyright 2013 Victor Smirnov
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

#include <memoria/core/tools/time.hpp>

#include "pseq_test_base.hpp"



namespace memoria {
namespace tests {

template <
    size_t Bits,
    typename IndexType,
    template <typename> class ReindexFnType = BitmapReindexFn,
    template <typename> class SelectFnType  = BitmapSelectFn,
    template <typename> class RankFnType    = BitmapRankFn,
    template <typename> class ToolsFnType   = BitmapToolsFn
>
class PackedSearchableSequenceSpeedTest: public PackedSearchableSequenceTestBase<
    Bits,
    IndexType,
    ReindexFnType,
    SelectFnType,
    RankFnType,
    ToolsFnType
> {

    typedef PackedSearchableSequenceSpeedTest<
            Bits,
            IndexType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           MyType;

    typedef PackedSearchableSequenceTestBase<
            Bits,
            IndexType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           Base;

    using typename Base::Seq;
    using typename Base::SeqPtr;

    using Value = typename Seq::Value;


    static const size_t Blocks                 = Seq::Indexes;
    static const size_t Symbols                = 1 << Bits;
    static const size_t VPB                    = Seq::ValuesPerBranch;


    using Base::getRandom;
    using Base::createEmptySequence;
    using Base::fillRandom;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::assertEmpty;
    using Base::out;


public:

    PackedSearchableSequenceSpeedTest()
    {
        this->iterations_   = 4096;
        this->size_         = 4096;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testInsertRemove);
    }


    void testInsertRemove()
    {
        auto seq = createEmptySequence();

        int64_t t0 = getTimeInMillis();

        fillRandom(seq, this->size_);

        int64_t t1 = getTimeInMillis();

        for (size_t c = 0; c < this->iterations_; c++)
        {
            size_t idx1 = getRandom(this->size_);
            size_t idx2 = getRandom(this->size_);

            size_t symbol = getRandom(Symbols);

            seq->remove(idx2, idx2 + 1).get_or_throw();
            seq->insert(idx1, symbol).get_or_throw();
        }

        int64_t t2 = getTimeInMillis();

        out() << FormatTime(t1 - t0) << " " << FormatTime(t2 - t1) << std::endl;
    }



};


}}
