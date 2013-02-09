
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_COUNT_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_COUNT_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceCountTest: public SPTestTask {

    typedef SequenceCountTest<BitsPerSymbol, Dense>                            	MyType;
    typedef SymbolSequence<BitsPerSymbol>										MemBuffer;

    typedef SPTestTask                                                          Base;

    typedef typename SCtrTF<Sequence<BitsPerSymbol, Dense>>::Type               Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Ctr::ID                                                    ID;

    typedef typename Ctr::ElementType											T;

    static const Int Symbols = 1 << BitsPerSymbol;

    Int ctr_name_;

    Int rank_ = 1;

public:
    SequenceCountTest(StringRef name):
        Base(name)
    {
        Base::size_ = 2048*1024;

        MEMORIA_ADD_TEST_PARAM(rank_)->minValue(1);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();


        MEMORIA_ADD_TEST(runIteratorSequentialCountNextTest);
        MEMORIA_ADD_TEST(runIteratorSequentialCountPrevTest);
    }

    vector<Int> fillSequence(Ctr& ctr, Int size)
    {
    	MemBuffer buffer(size);
    	buffer.resize(size);

    	buffer.fillCells([](T& cell) {
    		cell = 0;
    	});

    	vector<Int> sizes;

    	for (UInt idx = 0, length = 1; idx < buffer.size(); length += 100)
    	{
    		UInt cnt;
    		for (cnt = 0; cnt < length && idx < buffer.size() - 1; cnt++, idx++)
    		{
    			buffer[idx] = 1;
    		}

    		sizes.push_back(cnt);

    		idx++;
    	}

    	buffer.reindex();

    	auto src = buffer.source();

    	ctr.begin().insert(src);

    	return sizes;
    }


    void runIteratorSequentialCountNextTest(ostream& out)
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	vector<Int> sizes = fillSequence(ctr, Base::size_);

    	allocator.commit();

    	Int symbol 	= 1;

    	Int selections = 0;

    	BigInt t0 = getTimeInMillis();

    	auto iter = ctr.Begin();

    	Int idx = 0;

    	while (!iter.isEof())
    	{
    		Int size = iter.countNext(symbol);

    		AssertEQ(MA_SRC, iter.element(), 0ul);
    		AssertEQ(MA_SRC, size, sizes[idx]);

    		iter++;
    		idx++;
    	}

    	BigInt t1 = getTimeInMillis();
    	cout<<"FW time="<<FormatTime(t1 - t0)<<" selections="<<selections<<endl;
    }


    void runIteratorSequentialCountPrevTest(ostream& out)
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	vector<Int> sizes = fillSequence(ctr, Base::size_);

    	sizes.push_back(0);

    	allocator.commit();

    	Int symbol 	= 1;

    	Int selections = 0;

    	BigInt t0 = getTimeInMillis();

//    	auto iter1 = ctr.Begin();
//
//    	do
//    	{
//    		ctr.dump(iter1.data());
//    	}
//    	while (iter1.nextKey());

    	auto iter = ctr.End();

    	Int idx = sizes.size();

    	while(iter--)
    	{
    		idx--;

//    		cout<<iter.dataPos()<<" ";
    		Int size = iter.countPrev(symbol);
//    		cout<<iter.dataPos()<<" "<<size<<endl;

    		AssertEQ(MA_SRC, iter.element(), idx == 0);
    		AssertEQ(MA_SRC, size, sizes[idx], SBuf()<<idx);
    	}

    	BigInt t1 = getTimeInMillis();
    	cout<<"FW time="<<FormatTime(t1 - t0)<<" selections="<<selections<<endl;
    }

};




}


#endif
