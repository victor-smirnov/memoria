
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQ_PSEQ_RANK_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQ_PSEQ_RANK_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_seq.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <Int Bits>
class PSeqRankTest: public TestTask {

    typedef PSeqRankTest MyType;

    typedef PackedSeqTypes<
    		UInt,
    		UBigInt,
    		Bits,
    		PackedSeqBranchingFactor,
    		PackedSeqValuesPerBranch / (Bits * 2)
    > 																			Types;


    typedef typename Types::IndexKey        Key;
    typedef typename Types::Value           Value;

    typedef PackedSeq<Types>            	Seq;

    static const Int Blocks                 = Seq::Blocks;
    static const Int Symbols                = 1<<Seq::Bits;
    static const Int VPB					= Seq::ValuesPerBranch;

public:

    PSeqRankTest(): TestTask((SBuf()<<"Rank."<<Bits).str())
    {
//        MEMORIA_ADD_TEST(runTest1);
//        MEMORIA_ADD_TEST(runTest2);
//        MEMORIA_ADD_TEST(runTest3);

        MEMORIA_ADD_TEST(runTest4);
    }

    virtual ~PSeqRankTest() throw() {}

    Seq* createEmptySequence()
    {
    	Int buffer_size     = Bits < 8 ? 8192*Bits : 128*1024*Bits;

    	Byte* buffer       	= new Byte[buffer_size];
    	memset(buffer, 0, buffer_size);

    	Seq* seq           = T2T<Seq*>(buffer);
    	seq->initByBlock(buffer_size - sizeof(Seq));

    	return seq;
    }


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

    void assertRank(Seq* seq, size_t start, size_t end, Value symbol)
    {
    	Int rank = seq->rank(start, end, symbol);
    	Int popc = seq->popCount(start, end + 1, symbol);

    	AssertEQ(MA_SRC, rank, popc);
    }

    void assertRank(Seq* seq, size_t end, Value symbol)
    {
    	Int rank = seq->rank1(end, symbol);
    	Int popc = seq->rank1(0, end, symbol);

    	AssertEQ(MA_SRC, rank, popc, SBuf()<<end);
    }

    void runTest1(ostream& out)
    {
    	out<<"Parameters: Bits="<<Bits<<endl;

    	Seq* seq = createEmptySequence();

    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Symbols);
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();

    	assertRank(seq, 10,seq->size() - 10, 0);

    	auto starts = createStarts(seq);

    	for (size_t start: starts)
    	{
    		out<<start<<endl;

    		auto ends = createEnds(seq, start);

    		for (size_t end: ends)
    		{
    			assertRank(seq, start, end, 0);
    			assertRank(seq, start, end, Symbols - 1);
    		}
    	}
    }

    void runTest2(ostream& out)
    {
    	out<<"Parameters: Bits="<<Bits<<endl;

    	Seq* seq = createEmptySequence();

    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Blocks == 1 ? 2 : Blocks);
    	}

    	for (Int end = 1; end < seq->maxSize(); end++)
    	{
    		seq->size() = end;
    		seq->reindex();

    		assertRank(seq, 0, end - 1, 0);
    		assertRank(seq, 0, end - 1, Symbols - 1);
    	}
    }

    void runTest3(ostream& out)
    {
    	out<<"Parameters: Bits="<<Bits<<endl;

    	Seq* seq = createEmptySequence();

    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Symbols);
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();

    	Int stop = seq->size() - 1;

    	for (Int start = 0; start < seq->maxSize(); start++)
    	{
    		assertRank(seq, start, stop, 0);
    		assertRank(seq, start, stop, Symbols - 1);
    	}
    }

    void runTest4(ostream& out)
    {
    	out<<"Parameters: Bits="<<Bits<<endl;

    	Seq* seq = createEmptySequence();

    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Symbols);
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();

    	for (Int c = 0; c <= seq->size(); c++)
    	{
    		assertRank(seq, c, 0);
    	}
    }
};


}


#endif
