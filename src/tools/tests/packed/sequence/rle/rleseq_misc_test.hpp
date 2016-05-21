
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

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memory>

#include "rleseq_test_base.hpp"

namespace memoria {
namespace v1 {

using namespace std;

template <Int Symbols>
class PackedRLESearchableSequenceMiscTest: public PackedRLESequenceTestBase<Symbols> {

    using MyType = PackedRLESearchableSequenceMiscTest<Symbols>;
    using Base = PackedRLESequenceTestBase<Symbols>;


    using typename Base::Seq;
    using typename Base::SeqPtr;

    static const Int Blocks = Seq::Indexes;

    using Value = typename Seq::Value;

    using Base::getRandom;
    using Base::createEmptySequence;
    using Base::fillRandom;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::assertEmpty;
    using Base::out;


public:

    PackedRLESearchableSequenceMiscTest(StringRef name): Base(name)
    {
        this->size_ = 2048;

        MEMORIA_ADD_TEST(testCreate);
        MEMORIA_ADD_TEST(testInsertSingle);
//        MEMORIA_ADD_TEST(testInsertMultiple);
        MEMORIA_ADD_TEST(testRemoveMulti);
        MEMORIA_ADD_TEST(testRemoveAll);
        MEMORIA_ADD_TEST(testClear);
    }

    virtual ~PackedRLESearchableSequenceMiscTest() noexcept {}

    void testCreate()
    {
        for (Int size = 8; size <= this->size_; size *= 2)
        {
            out() << size << endl;

            auto seq = createEmptySequence();

            auto symbols = this->fillRandom(seq, size);

            assertIndexCorrect(MA_SRC, seq);
            assertEqual(seq, symbols);
        }
    }


    void testInsertSingle()
    {
        for (Int size = 1; size <= this->size_; size *= 2)
        {
            out() << size << endl;

            auto seq = createEmptySequence();

            auto symbols = fillRandom(seq, size);

            for (Int c = 0; c < this->iterations_; c++)
            {
                Int idx     = getRandom(seq->size());
                Int symbol  = getRandom(Blocks);

                out() << "Insert " << symbol << " at " << idx << endl;

                seq->insert(idx, symbol);

                symbols.insert(symbols.begin() + idx, symbol);

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);
            }
        }
    }
//
//    void testInsertMultiple()
//    {
//        for (Int size = 8192; size <= this->size_; size *= 2)
//        {
//            out()<<size<<std::endl;
//
//            auto seq = this->createEmptySequence();
//
//            auto symbols = fillRandom(seq, size);
//
//            for (Int c = 0; c < this->iterations_; c++)
//            {
//                Int idx     = getRandom(seq->size());
//
//                vector<Int> block(10);
//                for (Int d = 0; d < block.size(); d++)
//                {
//                    block[d] = getRandom(Blocks);
//                }
//
//                Int cnt = 0;
//                seq->insert(idx, block.size(), [&](){
//                    return block[cnt++];
//                });
//
//                symbols.insert(symbols.begin() + idx, block.begin(), block.end());
//
//                assertIndexCorrect(MA_SRC, seq);
//                assertEqual(seq, symbols);
//            }
//        }
//    }
//
    void testRemoveMulti()
    {
        for (Int size = 1; size <= this->size_; size *= 2)
        {
            out() << "Sequence size: " << size << std::endl;

            auto seq = createEmptySequence();

            auto symbols = fillRandom(seq, size);

            for (Int c = 0; c < this->iterations_; c++)
            {
                Int start   = getRandom(seq->size());
                Int end     = start + getRandom(seq->size() - start);

                out() << "Remove from " << start << " to " << end << endl;

                Int block_size = seq->block_size();

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
        for (Int size = 1; size <= this->size_; size *= 2)
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
        for (Int size = 1; size <= this->size_; size *= 2)
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
