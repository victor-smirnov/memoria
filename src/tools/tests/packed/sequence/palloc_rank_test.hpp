
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_RANK_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_RANK_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "palloc_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <
    Int Bits,
    template <typename> class IndexType     = PkdFTree,
    template <typename> class CodecType     = ValueFSECodec,
    template <typename> class ReindexFnType = BitmapReindexFn,
    template <typename> class SelectFnType  = BitmapSelectFn,
    template <typename> class RankFnType    = BitmapRankFn,
    template <typename> class ToolsFnType   = BitmapToolsFn
>
class PackedSearchableSequenceRankTest: public PackedSearchableSequenceTestBase<
    Bits,
    IndexType,
    CodecType,
    ReindexFnType,
    SelectFnType,
    RankFnType,
    ToolsFnType
> {

    typedef PackedSearchableSequenceRankTest<
            Bits,
            IndexType,
            CodecType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           MyType;

    typedef PackedSearchableSequenceTestBase<
            Bits,
            IndexType,
            CodecType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           Base;




    typedef typename Base::Seq                                                  Seq;

    typedef typename Seq::Value                                                 Value;


    static const Int Blocks                 = Seq::Indexes;
    static const Int Symbols                = 1<<Bits;
    static const Int VPB                    = Seq::ValuesPerBranch;

public:

    PackedSearchableSequenceRankTest(StringRef name): Base(name)
    {
        this->size_ = 8192;

        MEMORIA_ADD_TEST(runTest1);
        MEMORIA_ADD_TEST(runTest3);
        MEMORIA_ADD_TEST(runTest4);
    }

    virtual ~PackedSearchableSequenceRankTest() throw() {}



    vector<size_t> createStarts(const Seq* seq)
    {
        size_t max_block  = seq->size() / VPB + (seq->size() % VPB == 0 ? 0 : 1);

        vector<size_t> starts;

        for (size_t block = 0; block < max_block; block++)
        {
            size_t block_start = block * VPB;
            size_t block_end = block_start + VPB <= (size_t)seq->size() ? block_start + VPB : seq->size();

            starts.push_back(block_start);
            starts.push_back(block_start + 1);

            for (size_t d = 2; d < (size_t)VPB && block_start + d < block_end; d += 128)
            {
                starts.push_back(block_start + d);
            }

            starts.push_back(block_end - 1);
            starts.push_back(block_end);
        }

        return starts;
    }


    vector<size_t> createEnds(const Seq* seq, size_t start)
    {
        size_t max_block  = seq->size() / VPB + (seq->size() % VPB == 0 ? 0 : 1);

        vector<size_t> ranks;

        for (size_t block = start / VPB; block < max_block; block++)
        {
            size_t block_start = block * VPB;
            size_t block_end = block_start + VPB <= (size_t)seq->size() ? block_start + VPB : seq->size();

            appendPos(ranks, start, block_end, block_start);
            appendPos(ranks, start, block_end, block_start + 1);


            for (size_t d = 128; d < (size_t)VPB; d += 128)
            {
                if (block_start + d >= start  && block_start + d < block_end) {
                    appendPos(ranks, start, block_end, block_start + d);
                }
            }

            appendPos(ranks, start, block_end, block_end - 1);
            appendPos(ranks, start, block_end, block_end);
        }

        return ranks;
    }

    void appendPos(vector<size_t>& v, size_t start, size_t end, size_t pos)
    {
        if (pos >= start && pos < end)
        {
            v.push_back(pos);
        }
    }

    void assertRank(const Seq* seq, size_t start, size_t end, Value symbol)
    {
        Int local_rank  = seq->rank(start, end, symbol);
        Int popc        = this->rank(seq, start, end, symbol);

        AssertEQ(MA_SRC, local_rank, popc);
    }

    void assertRank(const Seq* seq, size_t end, Value symbol)
    {
        Int rank = seq->rank(end, symbol);
        Int popc = seq->rank(0, end, symbol);

        AssertEQ(MA_SRC, rank, popc, SBuf()<<end);
    }

    void runTest1()
    {
        this->out()<<"Parameters: Bits="<<Bits<<endl;

        Seq* seq = this->createEmptySequence();
        PARemover remover(seq);

        this->fillRandom(seq, this->size_);

        assertRank(seq, 10, seq->size() - 10, 0);

        auto starts = createStarts(seq);

        for (size_t start: starts)
        {
            this->out()<<start<<endl;

            auto ends = createEnds(seq, start);

            for (size_t end: ends)
            {
                assertRank(seq, start, end, 0);
                assertRank(seq, start, end, Symbols - 1);
            }
        }
    }

    void runTest3()
    {
        this->out()<<"Parameters: Bits="<<Bits<<endl;

        Seq* seq = this->createEmptySequence();
        PARemover remover(seq);

        this->fillRandom(seq, this->size_);

        Int stop = seq->size() - 1;

        for (Int start = 0; start < seq->size(); start++)
        {
            assertRank(seq, start, stop, 0);
            assertRank(seq, start, stop, Symbols - 1);
        }
    }

    void runTest4()
    {
        this->out()<<"Parameters: Bits="<<Bits<<endl;

        Seq* seq = this->createEmptySequence();
        PARemover remover(seq);

        this->fillRandom(seq, this->size_);

        for (Int c = 0; c <= seq->size(); c++)
        {
            assertRank(seq, c, 0);
        }
    }
};


}


#endif
