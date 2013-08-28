
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_CXMULTISEQ_HPP_
#define MEMORIA_TESTS_PALLOC_CXMULTISEQ_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>


#include <memoria/core/packed/packed_multisequence.hpp>
#include <memoria/core/packed/packed_wavelet_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedCxMultiSequenceTest: public TestTask {

    typedef PackedCxMultiSequenceTest                                       MyType;


    typedef PackedFSECxSequenceTypes<>                                      Types;

    typedef PackedCxMultiSequence<Types>                                    MultiSequence;
    typedef typename MultiSequence::Sequence                                Sequence;

    typedef shared_ptr<MultiSequence>                                       MultiSequencePtr;
    typedef shared_ptr<Sequence>                                            SequencePtr;

    typedef typename MultiSequence::SelectResult                            SelectResult;

public:

    PackedCxMultiSequenceTest(): TestTask("CxMultiSeq")
    {
        MEMORIA_ADD_TEST(testCreate);
    }

    virtual ~PackedCxMultiSequenceTest() throw() {}

    MultiSequencePtr createMultiSequence(Int block_size, Int free_space = 0)
    {
        free_space = PackedAllocator::roundDownBytesToAlignmentBlocks(free_space);

        Int sequence_block_size = MultiSequence::block_size(block_size);

        void* memory_block = malloc(sequence_block_size + free_space);
        memset(memory_block, 0, sequence_block_size + free_space);

        MultiSequence* seq = T2T<MultiSequence*>(memory_block);

        seq->init(sequence_block_size);

        seq->forceResize(free_space);

        return MultiSequencePtr(seq, free);
    }

    vector<SequencePtr> createRandomSequences(Int size, Int max_seq_size)
    {
        vector<SequencePtr> seqs(size);

        for (auto& s: seqs)
        {
            s = createRandomSequence(getRandom(max_seq_size - 1) + 1);
        }

        return seqs;
    }

    SequencePtr createSequence(Int sequence_size)
    {
        Int sequence_block_size = Sequence::block_size(sequence_size);

        void* memory_block = malloc(sequence_block_size);
        memset(memory_block, 0, sequence_block_size);

        Sequence* seq = T2T<Sequence*>(memory_block);

        seq->init(sequence_block_size);

        AssertGE(MA_SRC, seq->max_size(), sequence_size);

        return SequencePtr(seq, free);
    }


    SequencePtr createRandomSequence(Int sequence_size)
    {
        SequencePtr seq_ptr = createSequence(sequence_size);

        for (Int c = 0; c < seq_ptr->max_size(); c++)
        {
            Int symbol = getRandom(256);
            seq_ptr->insert(c, symbol);
        }

        AssertEQ(MA_SRC, seq_ptr->size(), seq_ptr->max_size());

        seq_ptr->reindex();

        return seq_ptr;
    }

    void insertSequences(MultiSequencePtr& mseq_ptr, const vector<SequencePtr>& seqs)
    {
        Int subseq_num = 0;

        out()<<"Building Multisequence"<<endl;

        for (const SequencePtr& seq_ptr: seqs)
        {
            out()<<subseq_num<<endl;

            mseq_ptr->appendSubsequence();

            for (Int c = 0; c < seq_ptr->size(); c++)
            {
                mseq_ptr->appendSymbol(subseq_num, seq_ptr->value(c));
            }

            subseq_num++;
        }

        out()<<"Building is done"<<endl;
    }

    vector<pair<Int, Int>> getRankedSymbols(const Sequence* seq, Int num_maxs)
    {
        vector<pair<Int, Int>> result;

        Int ranks[255];
        for (auto& s: ranks) s = 0;

        for (Int c = 0; c < seq->size(); c++)
        {
            UByte value = seq->value(c);
            ranks[value]++;
        }

        for (Int n = 0; n < num_maxs; n++)
        {
            Int max      = 0;
            Int max_rank = 0;

            for (Int c = 0; c < 255; c++)
            {
                if (ranks[c] > max_rank)
                {
                    max = c;
                    max_rank = ranks[c];
                }
            }

            result.push_back(pair<Int, Int>(max, max_rank));
            ranks[max] = 0;
        }

        return result;
    }


    void checkRanks(MultiSequencePtr& mseq_ptr, const vector<SequencePtr>& seqs)
    {
        MultiSequence* mseq = mseq_ptr.get();

        for (Int c = 0; c < (Int)seqs.size(); c++)
        {
            Sequence* seq = seqs[c].get();

            for (auto sym: getRankedSymbols(seq, 4))
            {
                for (Int idx = 0; idx < seq->size(); idx++)
                {
                    Int rank1 = seq->rank(idx, sym.second);
                    Int rank2 = mseq->rank(c, idx, sym.second);

                    AssertEQ(MA_SRC, rank1, rank2);
                }
            }
        }
    }

    void checkSelects(MultiSequencePtr& mseq_ptr, const vector<SequencePtr>& seqs)
    {
        MultiSequence* mseq = mseq_ptr.get();

        for (Int c = 0; c < (Int)seqs.size(); c++)
        {
            Sequence* seq = seqs[c].get();

            for (auto sym: getRankedSymbols(seq, 4))
            {
                auto r1 = seq->select(sym.second, sym.first);
                auto r2 = mseq->select(c, sym.second, sym.first);

                AssertEQ(MA_SRC, r1.rank(), r2.rank());
            }
        }
    }


    void testCreate()
    {
        MultiSequencePtr mseq_ptr = createMultiSequence(1050, 1000000);

        auto seqs = createRandomSequences(1000, 100);

        insertSequences(mseq_ptr, seqs);

        auto* seq = mseq_ptr->sequence();
        seq->reindex();
        mseq_ptr->dump(out(), false);

        checkRanks(mseq_ptr, seqs);
        checkSelects(mseq_ptr, seqs);

        out()<<mseq_ptr->free_space()<<endl;
        out()<<mseq_ptr->sequence()->size()<<" "<<mseq_ptr->sequence()->max_size()<<" ";
        out()<<mseq_ptr->labels()->size()<<" "<<mseq_ptr->labels()->max_size()<<endl;
    }


};


}


#endif
