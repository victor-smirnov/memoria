
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_SELECT_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_SELECT_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed2/packed_fse_searchable_seq.hpp>

#include <memory>
#include <vector>
#include <functional>

namespace memoria {

using namespace std;

template <
	Int Bits,
	template <typename>	class IndexType 	= PackedFSETree,
	template <typename>	class CodecType 	= ValueFSECodec,
	template <typename>	class ReindexFnType = BitmapReindexFn,
	template <typename>	class SelectFnType	= BitmapSelectFn,
	template <typename>	class RankFnType	= BitmapRankFn
>
class PackedSearchableSequenceSelectTest: public TestTask {

    typedef PackedSearchableSequenceSelectTest<
    		Bits,
    		IndexType,
    		CodecType,
    		ReindexFnType,
    		SelectFnType,
    		RankFnType
    > 																			MyType;

    typedef PackedFSESeachableSeqTypes<
    		Bits,
    		PackedTreeBranchingFactor,
    		512,
    		IndexType,
    		CodecType,
    		ReindexFnType,
    		SelectFnType,
    		RankFnType
    > 																			Types;

    typedef PackedFSESearchableSeq<Types>										Seq;

    typedef typename Seq::Value													Value;


    static const Int Blocks                 = Seq::Indexes;
    static const Int Symbols                = 1<<Bits;
    static const Int VPB					= Seq::ValuesPerBranch;

public:

    PackedSearchableSequenceSelectTest(StringRef name): TestTask(name)
    {
        size_ = 32768;

    	MEMORIA_ADD_TEST(runSelectFromFWTest);
        MEMORIA_ADD_TEST(runSelectFWTest);

        MEMORIA_ADD_TEST(runSelectBWTest);
    }

    virtual ~PackedSearchableSequenceSelectTest() throw() {}

    Seq* createEmptySequence(Int capacity)
    {
    	Int sequence_block_size = Seq::empty_size() +
    			PackedAllocator::roundUpBitToBytes(capacity * Bits) * 2;

    	Int block_size = PackedAllocator::block_size(sequence_block_size, 1);

    	void* memory_block = malloc(block_size);
    	memset(memory_block, 0, block_size);

    	PackedAllocator* allocator = T2T<PackedAllocator*>(memory_block);

    	allocator->init(block_size, 1);

    	allocator->allocateEmpty<Seq>(0);

    	Seq* seq = allocator->template get<Seq>(0);

    	return seq;
    }

    void remove(PackedAllocatable* seq)
    {
    	free(seq->allocator());
    }

    void fillRandom(Seq* seq, Int size)
    {
    	seq->insert(0, size, [](){
    		return getRandom(Blocks);
    	});
    }


    void populate(Seq* seq, Int size, Value value = 0)
    {
    	seq->clear();
    	seq->insert(0, size, [=](){
    		return value;
    	});
    }

    void populateRandom(Seq* seq, Int size)
    {
    	seq->clear();
    	fillRandom(seq, size);
    }

    Int rank(const Seq* seq, Int start, Int end, Int symbol)
    {
    	Int rank = 0;

    	for (Int c = start; c < end; c++)
    	{
    		rank += seq->test(c, symbol);
    	}

    	return rank;
    }

    SelectResult selectFW(const Seq* seq, size_t start, size_t rank, Value symbol)
    {
    	size_t total = 0;

    	for (size_t c = start; c < (size_t)seq->size(); c++)
    	{
    		total += seq->test(c, symbol);

    		if (total == rank)
    		{
    			return SelectResult(c, rank, true);
    		}
    	}

    	return SelectResult(seq->size(), total, total == rank);
    }


    SelectResult selectBW(const Seq* seq, size_t start, size_t stop, size_t rank, Value symbol)
    {
    	if (rank == 0)
    	{
    		return SelectResult(start, 0, true);
    	}

    	size_t total = 0;

    	for (size_t c = start + 1; c > stop; c--)
    	{
    		total += seq->test(c - 1, symbol);

    		if (total == rank)
    		{
    			return SelectResult(c - 1, rank, true);
    		}
    	}

    	return SelectResult(stop, total, total == rank);
    }

    void assertSelectFW(const Seq* seq, size_t start, size_t rank, Value symbol)
    {
    	auto result1 = seq->selectFw(start, symbol, rank);
    	auto result2 = selectFW(seq, start, rank, symbol);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start<<" "<<rank);

    	if (!result1.is_found())
    	{
    		AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start<<" "<<rank);
    	}
    	else {
    		AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start<<" "<<rank);
    	}
    }

    void assertSelectBW(const Seq* seq, size_t start, size_t rank, Value symbol)
    {
    	auto result1 = seq->selectBw(start, symbol, rank);
    	auto result2 = selectBW(seq, start, 0, rank, symbol);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start<<" "<<rank);

    	if (!result1.is_found())
    	{
    		AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start<<" "<<rank);
    	}
    	else {
    		AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start<<" "<<rank);
    	}
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

    		Int rank_delta = 128 / Bits;

    		for (Int d = rank_delta; d < VPB; d += rank_delta)
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
    		if (seq->test(c, symbol))
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

    	for (Int c = start; c >= 0; c--)
    	{
    		if (seq->test(c, symbol))
    		{
    			rank++;
    			ranks.push_back(Pair(rank, c));
    		}
    	}

    	return ranks;
    }

    void runSelectFromFWTest()
    {
    	runSelectFromFWTest(0);
    	runSelectFromFWTest(Symbols - 1);
    }

    void runSelectFromFWTest(Value symbol)
    {
    	out()<<"Parameters: Bits="<<Bits<<" symbol="<<(Int)symbol<<endl;

    	Seq* seq = createEmptySequence(size_);

    	populate(seq, size_, symbol);

    	vector<size_t> starts = createStarts(seq);

    	out()<<"Solid bitmap"<<endl;

    	for (size_t start: starts)
    	{
    		out()<<start<<endl;

    		vector<size_t> ranks = createRanks(seq, start);

    		for (size_t rank: ranks)
    		{
    			assertSelectFW(seq, start, rank, symbol);
    		}
    	}

    	out()<<endl;
    	out()<<"Random bitmap, random positions"<<endl;

    	populateRandom(seq, size_);

    	for (size_t start: starts)
    	{
    		out()<<start<<endl;

    		vector<size_t> ranks = createRanks(seq, start);

    		for (size_t rank: ranks)
    		{
    			assertSelectFW(seq, start, rank, symbol);
    		}
    	}

    	out()<<endl;
    	out()<<"Random bitmap, "<<(Int)symbol<<"-set positions"<<endl;

    	for (size_t start : starts)
    	{
    		auto pairs = createRanksFW(seq, start, symbol);

    		out()<<start<<endl;

    		for (auto pair: pairs)
    		{
    			auto result = seq->selectFw(start, symbol, pair.rank);

    			AssertTrue(MA_SRC, result.is_found());

    			if (!result.is_found())
    			{
    				AssertEQ(MA_SRC, result.rank(), pair.rank);
    			}
    			else {
    				AssertEQ(MA_SRC, result.idx(),  pair.idx);
    			}
    		}
    	}

    	out()<<endl;

    	remove(seq);
    }


    void runSelectFWTest()
    {
    	Seq* seq = createEmptySequence(size_);

    	populateRandom(seq, size_);

    	Int max_rank = seq->rank(1) + 1;

    	for (Int rank = 1; rank < max_rank; rank++)
    	{
    		auto result1 = seq->selectFw(0, 1, rank);
    		auto result2 = seq->selectFw(1, rank);

    		auto result3 = selectFW(seq, 0, rank, 1);

    		AssertEQ(MA_SRC, result1.is_found(), result2.is_found(), SBuf()<<rank);
    		AssertEQ(MA_SRC, result1.is_found(), result3.is_found(), SBuf()<<rank);

    		if (result1.is_found())
    		{
    			AssertEQ(MA_SRC, result1.idx(), result2.idx(), SBuf()<<rank);
    			AssertEQ(MA_SRC, result1.idx(), result3.idx(), SBuf()<<rank);
    		}
    		else {
    			AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<rank);
    			AssertEQ(MA_SRC, result1.rank(), result3.rank(), SBuf()<<rank);
    		}
    	}

    	remove(seq);
    }


    void runSelectBWTest()
    {
    	runSelectBWTest(0);
    	runSelectBWTest(Symbols - 1);
    }

    void runSelectBWTest(Value symbol)
    {
    	out()<<"Parameters: "<<Bits<<" "<<(Int)symbol<<endl;

    	Seq* seq = createEmptySequence(size_);

    	populate(seq, size_, symbol);

    	vector<size_t> starts = createStarts(seq);

    	starts.push_back(seq->size());

    	out()<<"Solid bitmap"<<endl;

    	for (size_t start: starts)
    	{
    		out()<<start<<endl;

    		vector<size_t> ranks = createRanks(seq, start);

    		for (size_t rank: ranks)
    		{
    			assertSelectBW(seq, start, rank, symbol);
    		}
    	}

    	out()<<endl;
    	out()<<"Random bitmap, random positions"<<endl;

    	populateRandom(seq, size_);

    	for (size_t start: starts)
    	{
    		out()<<start<<endl;

    		vector<size_t> ranks = createRanks(seq, start);

    		for (size_t rank: ranks)
    		{
    			assertSelectBW(seq, start, rank, symbol);
    		}
    	}

    	out()<<endl;
    	out()<<"Random bitmap, "<<(Int)symbol<<"-set positions"<<endl;

    	for (size_t start : starts)
    	{
    		if (start < seq->size())
    		{
    			out()<<start<<endl;

    			auto pairs = createRanksBW(seq, start, symbol);

    			for (auto pair: pairs)
    			{
    				SelectResult result = seq->selectBw(start, symbol, pair.rank);

    				AssertTrue(MA_SRC, result.is_found(), SBuf()<<start<<" "<<pair.rank);

    				if (!result.is_found())
    				{
    					AssertEQ(MA_SRC, result.rank(), pair.rank, SBuf()<<start<<" "<<pair.rank);
    				}
    				else {
    					AssertEQ(MA_SRC, result.idx(),  pair.idx, SBuf()<<start<<" "<<pair.rank);
    				}
    			}
    		}
    	}

    	out()<<endl;

    	size_t seq_rank = this->rank(seq, 0, seq->size(), symbol);
    	assertSelectBW(seq, seq->size(), seq_rank/2, symbol);

    	remove(seq);
    }
};


}


#endif
