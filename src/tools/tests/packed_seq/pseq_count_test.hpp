
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQ_PSEQ_COUNT_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQ_PSEQ_COUNT_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_seq.hpp>

#include <memory>
#include <vector>
#include <functional>

namespace memoria {

using namespace std;

template <Int Bits>
class PSeqCountTest: public TestTask {

    typedef PSeqCountTest<Bits> MyType;

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

    PSeqCountTest(): TestTask((SBuf()<<"Count."<<Bits).str())
    {
        MEMORIA_ADD_TEST(runCountFWTest);

//        MEMORIA_ADD_TEST(runCountBWTest);
    }

    virtual ~PSeqCountTest() throw() {}

    void populate(Seq* seq, Value value = 0)
    {
    	Int size = seq->maxSize();

    	for (Int c = size; c < size; c++)
    	{
    		seq->value(c) = value;
    	}

    	seq->size() = size;
    	seq->reindex();
    }

    void populateRandom(Seq* seq, Int longest_run, Value symbol)
    {
    	Int size = seq->size();

    	Value other_symbol = symbol < 1 ? 1 - symbol : symbol - 1;

    	for (Int c = 0; c < size;)
    	{
    		Int run_length = getRandom(longest_run);

    		Int d;
    		for (d = c; d < size && d < run_length; d++)
    		{
    			seq->value(d) = symbol;
    		}

    		if (d < size)
    		{
    			seq->value(d) = other_symbol;
    		}

    		c += run_length + 1;
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();
    }

    SelectResult countFW(const Seq* seq, Int start, Value symbol)
    {
    	Int total = 0;

    	Int block = seq->getValueBlockOffset();

    	for (Int c = start; c < seq->size(); c++)
    	{
    		if (seq->testb(block, c, symbol))
    		{
    			total++;
    		}
    		else {
    			return SelectResult(c, total, true);
    		}
    	}

    	return SelectResult(start + total, total, start + total < seq->size());
    }


    SelectResult selectBW(const Seq* seq, size_t start, size_t stop, size_t rank, Value symbol)
    {
    	if (rank == 0) {
    		return SelectResult(start, 0, true);
    	}

    	size_t total = 0;
    	Int block = seq->getValueBlockOffset();

    	for (size_t c = start; c > stop; c--)
    	{
    		total += seq->testb(block, c - 1, symbol);

    		if (total == rank)
    		{
    			return SelectResult(c - 1, rank, true);
    		}
    	}

    	return SelectResult(stop, total, total == rank);
    }

    void assertCountFW(const Seq* seq, size_t start, Value symbol)
    {
    	auto result1 = seq->countFW(start, symbol);
    	auto result2 = countFW(seq, start, symbol);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start);
    	AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start);
    	AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start);
    }

    void assertSelectBW(const Seq* seq, size_t start, size_t rank, Value symbol)
    {
    	auto result1 = seq->selectBW(start, symbol, rank);
    	auto result2 = selectBW(seq, start, 0, rank, symbol);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start<<" "<<rank);
    	AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start<<" "<<rank);
    	AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start<<" "<<rank);
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


    vector<size_t> createRanks(const Seq* seq, size_t start)
    {
    	size_t max_block  = seq->size() / VPB + (seq->size() % VPB == 0 ? 0 : 1);

    	vector<size_t> ranks;

    	for (size_t block = start / VPB; block < max_block; block++)
    	{
    		size_t block_start = block * VPB;
    		size_t block_end = block_start + VPB <= (size_t)seq->size() ? block_start + VPB : seq->size();

    		appendRank(ranks, block_start);
    		appendRank(ranks, block_start + 1);

    		for (size_t d = 128; d < (size_t)VPB; d += 128)
    		{
    			appendRank(ranks, block_start + d);
    		}

    		appendRank(ranks, block_end - 1);
    		appendRank(ranks, block_end);
    	}

    	return ranks;
    }

    void appendRank(vector<size_t>& v, size_t rank)
    {
    	if (rank > 0)
    	{
    		v.push_back(rank);
    	}
    }

    struct Pair {
    	size_t rank;
    	size_t idx;

    	Pair(size_t r, size_t i): rank(r), idx(i) {}
    };

    vector<Pair> createRanksFW(const Seq* seq, size_t start, Value symbol)
    {
    	vector<Pair> ranks;

    	size_t rank = 0;

    	for (size_t c = start; c < (size_t)seq->size(); c++)
    	{
    		if (seq->value(c) == symbol)
    		{
    			rank++;
    			ranks.push_back(Pair(rank, c));
    		}
    	}

    	return ranks;
    }

    vector<Pair> createRanksBW(const Seq* seq, size_t start, Value symbol)
	{
    	vector<Pair> ranks;

    	size_t rank = 0;

    	for (size_t c = start; c > 0; c--)
    	{
    		if (seq->value(c - 1) == symbol)
    		{
    			rank++;
    			ranks.push_back(Pair(rank, c - 1));
    		}
    	}

    	return ranks;
    }


    Seq* createEmptySequence() const
    {
    	Int buffer_size     = Bits < 8 ? 8192*Bits : 65536*Bits;

    	Byte* buffer       	= new Byte[buffer_size];

    	memset(buffer, 0, buffer_size);

    	Seq* seq = T2T<Seq*>(buffer);
    	seq->initByBlock(buffer_size - sizeof(Seq));

    	return seq;
    }

    void runCountFWTest(ostream& out)
    {
    	runCountFWTest(out, 0);
    	runCountFWTest(out, Symbols - 1);
    }

    void runCountFWTest(ostream& out, Value symbol)
    {
    	out<<"Parameters: Bits="<<Bits<<" symbol="<<symbol<<endl;

    	Seq* seq = createEmptySequence();

    	populate(seq, symbol);

    	vector<size_t> starts = createStarts(seq);

    	out<<"Solid bitmap"<<endl;

    	for (size_t start: starts)
    	{
    		out<<start<<endl;
    		assertCountFW(seq, start, symbol);
    	}

    	out<<endl;
    	out<<"Random bitmap, sequential positions"<<endl;

    	for (Int c = 0; c < 50; c++)
    	{
    		out<<"pass: "<<(c + 1)<<endl;
    		populateRandom(seq, 2048, symbol);

    		for (Int start = 0; start < seq->size(); start++)
    		{
    			assertCountFW(seq, start, symbol);
    		}
    	}

    	out<<endl;
    }


    void runCountBWTest(ostream& out)
    {
    	runCountBWTest(out, 0);
    	runCountBWTest(out, Symbols - 1);
    }

    void runCountBWTest(ostream& out, Value symbol)
    {
    	out<<"Parameters: "<<Bits<<" "<<symbol<<endl;

    	Seq* seq = createEmptySequence();

    	populate(seq, seq->maxSize(), symbol);

    	vector<size_t> starts = createStarts(seq);

    	starts.push_back(seq->maxSize());

    	out<<"Solid bitmap"<<endl;

    	for (size_t start: starts)
    	{
    		out<<start<<endl;

    		vector<size_t> ranks = createRanks(seq, start);

    		for (size_t rank: ranks)
    		{
    			assertSelectBW(seq, start, rank, symbol);
    		}
    	}

    	out<<endl;
    	out<<"Random bitmap, random positions"<<endl;

    	populateRandom(seq, seq->maxSize());

    	for (size_t start: starts)
    	{
    		out<<start<<endl;

    		vector<size_t> ranks = createRanks(seq, start);

    		for (size_t rank: ranks)
    		{
    			assertSelectBW(seq, start, rank, symbol);
    		}
    	}

    	out<<endl;
    	out<<"Random bitmap, "<<symbol<<"-set positions"<<endl;

    	for (size_t start : starts)
    	{
    		out<<start<<endl;

    		auto pairs = createRanksBW(seq, start, symbol);

    		for (auto pair: pairs)
    		{
    			SelectResult result = seq->selectBW(start, symbol, pair.rank);

    			AssertTrue(MA_SRC, result.is_found());

    			AssertEQ(MA_SRC, result.rank(), pair.rank, SBuf()<<start<<" "<<pair.rank);
    			AssertEQ(MA_SRC, result.idx(),  pair.idx, SBuf()<<start<<" "<<pair.rank);
    		}
    	}

    	out<<endl;

    	size_t rank = seq->popCount(0, seq->size(), symbol);
    	assertSelectBW(seq, seq->size(), rank/2, symbol);
    }
};


}


#endif
