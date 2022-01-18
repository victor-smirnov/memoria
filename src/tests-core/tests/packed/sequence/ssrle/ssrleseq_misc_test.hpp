
// Copyright 2022 Victor Smirnov
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

#include <memory>

#include "ssrleseq_test_base.hpp"

namespace memoria {
namespace tests {

template <int32_t Symbols>
class PackedSSRLESearchableSequenceMiscTest: public PackedSSRLESequenceTestBase<Symbols> {

    using MyType = PackedSSRLESearchableSequenceMiscTest<Symbols>;
    using Base = PackedSSRLESequenceTestBase<Symbols>;

    using typename Base::SymbolsRunT;
    using typename Base::RunTraits;

    using typename Base::Seq;    
    using typename Base::SeqPtr;

    using Value = typename Seq::Value;

    using Base::getRandom;
    using Base::getRandom1;
    using Base::make_empty_sequence;
    using Base::make_random_sequence;
    using Base::make_sequence;
    using Base::assert_spans_equal;
    using Base::push_back;
    using Base::split_runs;
    using Base::out;
    using Base::size_;
    using Base::iterations_;


public:

    PackedSSRLESearchableSequenceMiscTest()
    {
        this->size_ = 2048;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testRunSequence, testCreate);
    }


    void testRunSequence()
    {
        auto seq1 = make_random_sequence(10240000);
        std::vector<SymbolsRunT> seq2 = split_runs(seq1);
        assert_spans_equal(seq1, seq2);
    }

    void testCreate()
    {
        println("Select: {}", RunTraits::bitmap_select(0xf0, 5));

//        std::vector<SymbolsRunT> syms1 = make_random_sequence(10240);
//        SeqPtr seq = make_sequence(syms1);
//        std::vector<SymbolsRunT> syms2 = seq->iterator().as_vector();
//        assert_equals(syms1, syms2);
    }

/*    void testCreate()
    {
        for (int32_t size = 64; size <= this->size_; size *= 2)
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
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            out() << size << std::endl;

            auto seq = createEmptySequence();
            auto symbols = fillRandom(seq, size);

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                int32_t idx     = getRandom(seq->size());
                int32_t symbol  = getRandom(Blocks);

                out() << "Insert " << symbol << " at " << idx << std::endl;

                seq->insert(idx, symbol).get_or_throw();

                symbols.insert(symbols.begin() + idx, symbol);

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);
            }
        }
    }

    void testSplit()
    {
        for (int32_t size = 16; size <= this->size_; size *= 2)
        {
            out() << size << std::endl;

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                auto seq = createEmptySequence();
                auto symbols = fillRandom(seq, size);

                int32_t idx = getRandom(seq->size());

                auto seq2 = createEmptySequence();

                out() << "Split at " << idx << std::endl;

                seq->splitTo(seq2.get(), idx).get_or_throw();

                seq2->check().get_or_throw();
                seq->check().get_or_throw();

                std::vector<int32_t> symbols2(symbols.begin() + idx, symbols.end());

                symbols.erase(symbols.begin() + idx, symbols.end());


                assertEqual(seq, symbols);
                assertEqual(seq2, symbols2);
            }
        }
    }

    void testMerge()
    {
        for (int32_t size = 16; size <= this->size_; size *= 2)
        {
            out() << size << std::endl;

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                auto seq1 = createEmptySequence();
                auto symbols1 = fillRandom(seq1, size);

                auto seq2 = createEmptySequence();
                auto symbols2 = fillRandom(seq2, size);
                
                //out() << "Seq1" << std::endl;
                seq1->dump(out());
                this->dumpAsSymbols(symbols1);
                
                //out() << "Seq2" << std::endl;
                seq2->dump(out());
                this->dumpAsSymbols(symbols2);

                seq1->mergeWith(seq2.get()).get_or_throw();

                //out() << "Seq1" << std::endl;
                seq1->dump(out());
                
                //out() << "Seq2" << std::endl;
                seq2->dump(out());
                
                seq2->check().get_or_throw();

                symbols2.insert(symbols2.end(), symbols1.begin(), symbols1.end());

                assertEqual(seq2, symbols2);
            }
        }
    }



    void testRemoveMulti()
    {
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            out() << "Sequence size: " << size << std::endl;

            auto seq = createEmptySequence();

            auto symbols = fillRandom(seq, size);

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                int32_t start   = getRandom(seq->size());
                int32_t end     = start + getRandom(seq->size() - start);

                out() << "Remove from " << start << " to " << end << std::endl;

                int32_t block_size = seq->block_size();

                seq->remove(start, end).get_or_throw();

                symbols.erase(symbols.begin() + start, symbols.begin() + end);

                assertIndexCorrect(MA_SRC, seq);
                assertEqual(seq, symbols);

                assert_le(seq->block_size(), block_size);
            }
        }
    }

    void testRemoveAll()
    {
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            this->out() << size << std::endl;

            auto seq = this->createEmptySequence();

            auto symbols = this->fillRandom(seq, size);
            assertEqual(seq, symbols);

            seq->remove(0, seq->size()).get_or_throw();

            assertEmpty(seq);
        }
    }

    void testClear()
    {
        for (int32_t size = 1; size <= this->size_; size *= 2)
        {
            this->out() << size << std::endl;

            auto seq = this->createEmptySequence();

            assertEmpty(seq);

            fillRandom(seq, size);

            assert_gt(seq->size(), 0);
            assert_gt(seq->block_size(), Seq::empty_size());

            seq->clear().get_or_throw();

            assertEmpty(seq);
        }
    }
*/
};


}}
