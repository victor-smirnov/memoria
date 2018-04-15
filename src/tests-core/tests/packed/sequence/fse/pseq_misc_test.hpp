
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/tools.hpp>

#include <memory>

#include "pseq_test_base.hpp"

namespace memoria {
namespace v1 {


template <
    int32_t Bits,
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

    static const int32_t Blocks                 = Seq::Indexes;
    static const int32_t Symbols                = 1<<Bits;
    static const int32_t VPB                    = Seq::ValuesPerBranch;

    using Value = typename Seq::Value;

    using Base::getRandom;
    using Base::createEmptySequence;
    using Base::fillRandom;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::assertEmpty;
    using Base::out;


public:

    PackedSearchableSequenceMiscTest(U16StringRef name): Base(name)
    {
        this->size_ = 8192;

        MEMORIA_ADD_TEST(testCreate);
        MEMORIA_ADD_TEST(testInsertSingle);
        MEMORIA_ADD_TEST(testInsertMultiple);
        MEMORIA_ADD_TEST(testRemoveMulti);
        MEMORIA_ADD_TEST(testRemoveAll);
        MEMORIA_ADD_TEST(testClear);
    }

    virtual ~PackedSearchableSequenceMiscTest() noexcept {}

    void testCreate()
    {
        for (int32_t size = 2048; size <= this->size_; size *= 2)
        {
            out()<<size<<std::endl;

            auto seq = createEmptySequence();

            auto symbols = this->fillRandom(seq, size);

            assertIndexCorrect(MA_SRC, seq);
            assertEqual(seq, symbols);
        }
    }


    void testInsertSingle()
    {
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            out()<<size<<std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = fillRandom(seq, size);

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                int32_t idx     = getRandom(seq->size());
                int32_t symbol  = getRandom(Blocks);

                seq->insert(idx, symbol);

                symbols.insert(symbols.begin() + idx, symbol);

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);
            }
        }
    }

    void testInsertMultiple()
    {
        for (int32_t size = 8192; size <= this->size_; size *= 2)
        {
            out()<<size<<std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = fillRandom(seq, size);

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                int32_t idx     = getRandom(seq->size());

                vector<int32_t> block(10);
                for (int32_t d = 0; d < block.size(); d++)
                {
                    block[d] = getRandom(Blocks);
                }

                int32_t cnt = 0;
                seq->insert(idx, block.size(), [&](){
                    return block[cnt++];
                });

                symbols.insert(symbols.begin() + idx, block.begin(), block.end());

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);
            }
        }
    }

    void testRemoveMulti()
    {
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            out()<<size<<std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = this->fillRandom(seq, size);

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                int32_t start   = getRandom(seq->size());
                int32_t end     = start + getRandom(seq->size() - start);

                int32_t block_size = seq->block_size();

                seq->remove(start, end);

                symbols.erase(symbols.begin() + start, symbols.begin() + end);

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);

                AssertLE(MA_SRC, seq->block_size(), block_size);
            }
        }
    }

    void testRemoveAll()
    {
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            this->out()<<size<<std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = this->fillRandom(seq, size);
            assertEqual(seq, symbols);

            seq->remove(0, seq->size());

            assertEmpty(seq);
        }
    }

    void testClear()
    {
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            this->out()<<size<<std::endl;

            auto seq = this->createEmptySequence();

            assertEmpty(seq);

            fillRandom(seq, size);

            AssertNEQ(MA_SRC, seq->size(), 0);
            AssertGT(MA_SRC, seq->block_size(), Seq::empty_size());

            seq->clear();

            assertEmpty(seq);
        }
    }

};


}}
