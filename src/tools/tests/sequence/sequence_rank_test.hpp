
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_RANK_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_RANK_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceRankTest: public SPTestTask{

    typedef SequenceRankTest<BitsPerSymbol, Dense>                            MyType;
    typedef SymbolSequence<BitsPerSymbol>										MemBuffer;

    typedef SPTestTask                                                          Base;

    typedef typename SCtrTF<Sequence<BitsPerSymbol, Dense>>::Type               Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Ctr::ID                                                    ID;

    typedef typename Ctr::ElementType											T;

    static const Int Symbols = 1 << BitsPerSymbol;

    Int ctr_name_;


public:
    SequenceRankTest(StringRef name):
        Base(name)
    {
        Base::size_ = 2048*1024;

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();

//        MEMORIA_ADD_TEST(runRank1Test);
        MEMORIA_ADD_TEST(runRankTest);
    }

    void fillRandom(Ctr& ctr, Int size)
    {
    	MemBuffer buffer(size);
    	buffer.resize(size);

    	buffer.fillCells([](T& cell) {
    		cell = getBIRandom();
    	});

    	auto src = buffer.source();

    	ctr.begin().insert(src);
    }

    BigInt rank(Ctr& ctr, Int from, Int to, Int symbol)
    {
    	Iterator iter = ctr.seek(from);

    	Int 	total_size 	= 0;
    	BigInt 	rank 		= 0;
    	BigInt  size = to - from;

    	while (!iter.isEof())
    	{
    		Int data_size = iter.data()->size();
    		Int start = iter.dataPos();

    		if (total_size + data_size < size)
    		{
    			rank += iter.data()->sequence().rank1(start, data_size, symbol);
    		}

    		else {
    			Int local_size = size - total_size;
    			rank += iter.data()->sequence().rank1(start, local_size, symbol);

    			break;
    		}

    		total_size += data_size;

    		iter.nextKey();
    	}

    	return rank;
    }

    void assertRank(Ctr& ctr, Int from, Int to, Int symbol)
    {
//    	BigInt rank0 = from == 0 ? 0 : ctr.rank(from - 1, symbol);
//
//    	BigInt rank1 = ctr.rank(to, symbol);
//    	BigInt rank2 = rank(ctr, from, to, symbol);

    	Iterator iter = ctr.seek(from);

    	BigInt distance = to - from;

    	BigInt rank1 = iter.rank(distance, symbol);

//    	BigInt rank2 = rank(ctr, from, to, symbol);

    	BigInt rank2 = ctr.rank(to, symbol) - ctr.rank(from, symbol);

    	AssertEQ(MA_SRC, rank1, rank2);
    }

    void runRank1Test()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	allocator.commit();

    	assertRank(ctr, ctr.size() / 4, ctr.size() * 3 / 4, 1);
    }


    void runRankTest()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	allocator.commit();

    	for (int c = 1, cnt = 0; c < ctr.size(); c += 100, cnt++)
    	{
    		out()<<cnt<<endl;
    		assertRank(ctr, 0, c, 0);
    	}

    	assertRank(ctr, 0, ctr.size() - 1, 0);
    }
};




}


#endif