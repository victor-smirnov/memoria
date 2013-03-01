
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_SELECT_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_SELECT_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceSelectTest: public SPTestTask{

    typedef SequenceSelectTest<BitsPerSymbol, Dense>                            MyType;
    typedef SymbolSequence<BitsPerSymbol>										MemBuffer;

    typedef SPTestTask                                                          Base;

    typedef typename SCtrTF<Sequence<BitsPerSymbol, Dense>>::Type               Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Ctr::ID                                                    ID;

    typedef typename Ctr::ElementType											T;

    static const Int Symbols = 1 << BitsPerSymbol;

    Int ctr_name_;
    String dump_name_;

    Int rank_ = 1;

public:
    SequenceSelectTest(StringRef name):
        Base(name)
    {
        Base::size_ = 2048*1024;

        MEMORIA_ADD_TEST_PARAM(rank_)->minValue(1);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();

        MEMORIA_ADD_TEST(runSelectTest);
        MEMORIA_ADD_TEST(runIteratorSequentialSelectNextTest);
        MEMORIA_ADD_TEST(runIteratorSequentialSelectPrevTest);
    }

    void checkIterator(Iterator& iter)
    {
    	Accumulator acc;
    	iter.ComputePrefix(acc);

    	AssertEQ(MA_SRC, acc[0], iter.prefix());
    }

    void fillRandom(Ctr& ctr, Int size)
    {
    	MemBuffer buffer(size);
    	buffer.resize(size);

    	buffer.fillCells([](T& cell) {
    		cell = getBIRandom();
    	});

    	buffer[0] 		 = 1;
    	buffer[size - 1] = 1;

    	auto src = buffer.source();

    	ctr.begin().insert(src);
    }

    SelectResult selectFW(Ctr& ctr, Int rank, Int symbol)
    {
    	Iterator iter = ctr.begin();

    	BigInt 	total_rank 		= 0;

    	while (!iter.isEof())
    	{
    		Int start 		= iter.dataPos();

    		auto result 	= iter.data()->sequence().selectFW(start, rank - total_rank, symbol);

    		if (result.is_found())
    		{
    			return SelectResult(result.idx() + iter.prefix(0), rank, true);
    		}

    		total_rank 		+= result.rank();

    		iter.nextKey();
    	}

    	return SelectResult(ctr.size(), total_rank, false);
    }

    struct Pair {
    	Int rank;
    	Int idx;
    	Pair(Int r, Int i): rank(r), idx(i) {}
    };

    vector<Pair> getSelections(Ctr& ctr, Int from, Int to, Int symbol)
	{
    	Iterator iter = ctr.seek(from);

    	Int 	total_size 	= 0;
    	BigInt 	rank 		= 0;
    	BigInt  size 		= to - from;
    	Int 	step 		= 10;

    	auto& data 			= iter.data();

    	vector<Pair> pairs;

    	while (!iter.isEof())
    	{
    		Int data_size	= data->size();
    		Int start 		= iter.dataPos();

    		const auto& sequence = data->sequence();

    		Int local_rank = 1;

    		UInt size_limit = (total_size + data_size <= size) ? data_size : (size - total_size);

    		while (true)
    		{
    			auto result = sequence.selectFW(start, symbol, local_rank);

    			if (result.is_found())
    			{
    				pairs.push_back(Pair(rank + local_rank, result.idx() + iter.prefix(0)));
    				local_rank += step;
    			}
    			else {
    				local_rank = result.rank();

    				Int rank0 = sequence.rank(0, sequence.size()-1, symbol);

    				rank += rank0;

    				break;
    			}

    			if (result.idx() >= size_limit)
    			{
    				break;
    			}
    		}

    		if (total_size + data_size >= size)
    		{
    			break;
    		}

    		total_size += data_size;

    		iter.nextKey();
    	}

    	return pairs;
	}

    void assertSelect1(Ctr& ctr, Int rank, Int pos, Int symbol)
    {
    	Iterator iter = ctr.select(rank, symbol);
    	BigInt rank1 = ctr.rank(iter.pos() + 1, symbol);

    	AssertTrue(MA_SRC, !iter.isEof());

    	AssertEQ(MA_SRC, rank1, rank);
    	AssertEQ(MA_SRC, iter.pos(), pos);
    }

    void assertSelect2(Ctr& ctr, Int rank, Int pos, Int symbol)
    {
//    	BigInt idx = ctr.selectIdx(rank, symbol);
//    	BigInt rank1 = ctr.rank(idx + 1, symbol);
//
//    	AssertEQ(MA_SRC, rank1, rank);
//    	AssertEQ(MA_SRC, idx, pos);
    }

    void runSelectTest()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	allocator.commit();

    	vector<Pair> ranks = getSelections(ctr, 0, ctr.size(), 1);

    	vector<Int> cells(ranks.size());

    	for (Int& cell: cells)
    	{
    		cell = getRandom(cells.size());
    	}

    	out()<<"Selections: "<<ranks.size()<<endl;

    	BigInt t0 = getTimeInMillis();

//    	for (Pair pair: ranks)
//    	{
//    		assertSelect(ctr, pair.rank, pair.idx, 1);
//    	}

    	for (Int cell: cells)
    	{
    		Pair& pair = ranks[cell];
    		assertSelect1(ctr, pair.rank, pair.idx, 1);
    	}

    	BigInt t1 = getTimeInMillis();

    	for (Int cell: cells)
    	{
    		Pair& pair = ranks[cell];
    		assertSelect2(ctr, pair.rank, pair.idx, 1);
    	}

    	BigInt t2 = getTimeInMillis();

    	out()<<"times = "<<FormatTime(t1 - t0)<<" "<<FormatTime(t2 - t1)<<endl;
    }

    void runSelect1Test(ostream& out)
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	allocator.commit();

    	dump_name_ = Store(allocator);

    	Int symbol = 1;

    	BigInt pos = ctr.size();

    	BigInt rank = ctr.rank(pos, symbol);
//    	Iterator iter1 = ctr.select(rank, symbol);
    	Iterator iter2 = ctr.RBegin();

    	iter2.selectBw(rank -1, symbol);

//    	AssertEQ(MA_SRC, iter1.pos(), ctr.size() - 1);
    	AssertGT(MA_SRC, iter2.pos(), 0);
    }


    void runIteratorSequentialSelectNextTest()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	allocator.commit();

    	Int symbol 	= 1;

    	Int selections = 0;

    	BigInt t0 = getTimeInMillis();

    	for (Int rank = rank_; rank < 1000000; rank += 1000)
    	{
    		auto iter = ctr.begin();

    		while ((!iter.isEof()) && iter.selectFw(rank, symbol) == rank)
    		{
    			checkIterator(iter);

    			Int s = iter.element();

    			AssertEQ(MA_SRC, s, symbol);

    			iter++;
    			selections++;
    		}
    	}

    	BigInt t1 = getTimeInMillis();
    	out()<<"FW time="<<FormatTime(t1 - t0)<<" selections="<<selections<<endl;
    }

    void runIteratorSequentialSelectPrevTest()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	allocator.commit();

    	Int symbol 	= 1;

    	Int selections = 0;

    	BigInt t0 = getTimeInMillis();

    	for (Int rank = rank_; rank < 100000; rank += 1000)
    	{
    		auto iter = ctr.RBegin();

    		while (iter.selectBw(rank, symbol) == rank)
    		{
    			checkIterator(iter);

    			Int s = iter.element();

    			AssertEQ(MA_SRC, s, symbol);

    			if (!iter--) break;
    			selections++;
    		}
    	}

    	BigInt t1 = getTimeInMillis();
    	out()<<"BW time="<<FormatTime(t1 - t0)<<" selections="<<selections<<endl;
    }
};




}


#endif
