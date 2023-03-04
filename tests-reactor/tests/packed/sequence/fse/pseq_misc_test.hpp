
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

#include "pseq_test_base.hpp"

#include <memory>


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
class PackedSearchableSequenceMiscTest: public PackedSearchableSequenceTestBase<
    Bits,
    IndexType,
    ReindexFnType,
    SelectFnType,
    RankFnType,
    ToolsFnType
> {

    typedef PackedSearchableSequenceMiscTest<
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

    static const size_t Blocks                 = Seq::Indexes;
    static const size_t Symbols                = 1 << Bits;
    static const size_t VPB                    = Seq::ValuesPerBranch;

    using Value = typename Seq::Value;

    using Base::getRandom;
    using Base::createEmptySequence;
    using Base::fillRandom;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::assertEmpty;
    using Base::out;
    using Base::size_;
    using Base::iterations_;


public:

    PackedSearchableSequenceMiscTest()
    {
        this->size_ = 8192;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testCreate, testInsertSingle, testInsertMultiple, testRemoveMulti);
        MMA_CLASS_TESTS(suite, testRemoveAll, testClear);
    }


    void testCreate()
    {
        for (size_t size = 2048; size <= this->size_; size *= 2)
        {
            out() << size << std::endl;

            auto seq = createEmptySequence();

            auto symbols = this->fillRandom(seq, size);

            assertIndexCorrect(MA_SRC, seq);
            assertEqual(seq, symbols);
        }
    }


    void testInsertSingle()
    {
        for (size_t size = 1; size <= this->size_; size *= 2)
        {
            out() << size << std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = fillRandom(seq, size);

            for (size_t c = 0; c < this->iterations_; c++)
            {
                size_t idx     = getRandom(seq->size());
                size_t symbol  = getRandom(Blocks);

                seq->insert(idx, symbol);

                symbols.insert(symbols.begin() + idx, symbol);

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);
            }
        }
    }

    void testInsertMultiple()
    {
        for (size_t size = 8192; size <= this->size_; size *= 2)
        {
            out() << size << std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = fillRandom(seq, size);

            for (size_t c = 0; c < this->iterations_; c++)
            {
                size_t idx     = getRandom(seq->size());

                std::vector<size_t> block(10);
                for (size_t d = 0; d < block.size(); d++)
                {
                    block[d] = getRandom(Blocks);
                }

                size_t cnt = 0;
                seq->insert(idx, block.size(), [&](){
                    return block[cnt++];
                }).get_or_throw();

                symbols.insert(symbols.begin() + idx, block.begin(), block.end());

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);
            }
        }
    }

    void testRemoveMulti()
    {
        for (size_t size = 1; size <= this->size_; size *= 2)
        {
            out() << size << std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = this->fillRandom(seq, size);

            for (size_t c = 0; c < this->iterations_; c++)
            {
                size_t start   = getRandom(seq->size());
                size_t end     = start + getRandom(seq->size() - start);

                size_t block_size = seq->block_size();

                seq->remove(start, end);

                symbols.erase(symbols.begin() + start, symbols.begin() + end);

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);

                assert_le(seq->block_size(), block_size);
            }
        }
    }

    void testRemoveAll()
    {
        for (size_t size = 1; size <= this->size_; size *= 2)
        {
            this->out() << size << std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = this->fillRandom(seq, size);
            assertEqual(seq, symbols);

            seq->remove(0, seq->size());

            assertEmpty(seq);
        }
    }

    void testClear()
    {
        for (size_t size = 1; size <= this->size_; size *= 2)
        {
            this->out() << size << std::endl;

            auto seq = this->createEmptySequence();

            assertEmpty(seq);

            fillRandom(seq, size);

            assert_gt(seq->size(), 0);
            assert_gt(seq->block_size(), Seq::empty_size());

            seq->clear();

            assertEmpty(seq);
        }
    }

};


}}
