
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_ITER_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_ITER_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>
#include <functional>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceIteratorTest: public SPTestTask{

    typedef SequenceIteratorTest<BitsPerSymbol, Dense>                          MyType;
    typedef PackedFSESequence<BitsPerSymbol>										MemBuffer;

    typedef SPTestTask                                                          Base;

    typedef typename SCtrTF<Sequence<BitsPerSymbol, Dense>>::Type               Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Ctr::ID                                                    ID;

    typedef typename Ctr::ElementType											T;

    static const Int Symbols = 1 << BitsPerSymbol;

    Int ctr_name_;

    BigInt size2_;

    typedef std::function<void (MyType*, Iterator)> SkipFn;

public:
    SequenceIteratorTest(StringRef name):
        Base(name)
    {
        size2_ = Base::size_ = 2048*1024;

        MEMORIA_ADD_TEST_PARAM(size2_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();

        MEMORIA_ADD_TEST(runRangeTest);
        MEMORIA_ADD_TEST(runSkipFwTest);
        MEMORIA_ADD_TEST(runSkipBwTest);
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


    void runRangeTest()
    {
    	DefaultLogHandlerImpl logHandler(out());
    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	try {
    		Ctr ctr(&allocator);

    		ctr_name_ = ctr.name();

    		Iterator i = ctr.Begin();

    		AssertTrue(MA_SRC, i.isEof());
    		AssertTrue(MA_SRC, i.isEmpty());

    		Int size = 100000;

    		fillRandom(ctr, size);

    		allocator.commit();

    		Store(allocator);

    		Iterator i1 = ctr.Begin();
    		AssertEQ(MA_SRC, i1.pos(), 0);

    		Iterator i2 = ctr.End();
    		AssertFalse(MA_SRC, i2.isEnd());
    		AssertTrue(MA_SRC, i2.isEof());

    		Iterator i3 = ctr.REnd();
    		AssertEQ(MA_SRC, i3.key_idx(), 0);
    		AssertEQ(MA_SRC, i3.pos(), -1);

    		Iterator i4 = ctr.RBegin();
    		AssertLE(MA_SRC, i4.key_idx(), i4.page()->children_count());
    		AssertGT(MA_SRC, i4.pos(), 0);
    		AssertEQ(MA_SRC, i4.dataPos(), i4.data()->size() - 1);

    		Int element_count = 0;
    		for (auto v: ctr)
    		{
    			element_count++;
    		}

    		AssertEQ(MA_SRC, element_count, size);
    	}
    	catch (...) {
    		throw;
    	}
    }

    BigInt getPrefix(Iterator& iter) {
    	Accumulator acc;
    	iter.ComputePrefix(acc);

    	return acc[0];
    }

    BigInt getPosition(Iterator& iter)
    {
    	return getPrefix(iter) + iter.dataPos();
    }

    void assertIteratorPosition(const char* source, Iterator& iter)
    {
    	AssertEQ(source, iter.pos(), getPosition(iter));
    }

    void assertIteratorEQ(const char* source, Iterator& iter1, Iterator& iter2)
    {
    	AssertEQ(source, iter1.pos(), iter2.pos(), SBuf()<<"Positions");
    	AssertEQ(source, iter1.prefix(), iter2.prefix(), SBuf()<<"Prefixes");
    	AssertEQ(source, iter1.dataPos(), iter2.dataPos(), SBuf()<<"Local Positions");
    	AssertEQ(source, iter1.data()->id(), iter2.data()->id(), SBuf()<<"Pages");
    }

    vector<Int> createPositions(Ctr& ctr)
    {
    	vector<Int> v;

    	for (auto iter = ctr.Begin(); !iter.isEnd(); iter.nextKey())
    	{
    		Int page_size = iter.data()->size();

    		v.push_back(getPosition(iter));

    		iter.skip(page_size/2);
    		v.push_back(getPosition(iter));

    		iter.skip(page_size - page_size/2 - 1);
    		v.push_back(getPosition(iter));
    	}

    	return v;
    }




    void runSkipFwTest()
    {
    	DefaultLogHandlerImpl logHandler(out());
    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	vector<Int> positions = createPositions(ctr);

    	positions.push_back(ctr.size());

    	for (UInt start = 0; start < positions.size(); start++)
    	{
    		for (UInt end = start; end < positions.size(); end++)
    		{
    			Int start_pos = positions[start];
    			Int end_pos = positions[end];

    			Iterator iter_start = ctr.seek(start_pos);

    			assertIteratorPosition(MA_SRC, iter_start);
    			AssertEQ(MA_SRC, iter_start.pos(), start_pos);


    			Iterator iter = iter_start;

    			iter.skipFw(end_pos - start_pos);

    			assertIteratorPosition(MA_SRC, iter);
    			AssertEQ(MA_SRC, iter.pos(), end_pos);

    			Iterator iter_end = ctr.seek(end_pos);

    			assertIteratorEQ(MA_SRC, iter, iter_end);
    		}
    	}
    }


    void runSkipBwTest()
    {
    	DefaultLogHandlerImpl logHandler(out());
    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	fillRandom(ctr, Base::size_);

    	vector<Int> positions = createPositions(ctr);

    	positions.push_back(ctr.size());

    	for (Int start = positions.size() - 1; start >= 0; start --)
    	{
    		for (Int end = start; end >= 0; end--)
    		{
    			Int start_pos = positions[start];
    			Int end_pos = positions[end];

    			Iterator iter_start = ctr.seek(start_pos);

    			assertIteratorPosition(MA_SRC, iter_start);
    			AssertEQ(MA_SRC, iter_start.pos(), start_pos);

    			Iterator iter = iter_start;

    			iter.skipBw(start_pos - end_pos);

    			assertIteratorPosition(MA_SRC, iter);
    			AssertEQ(MA_SRC, iter.pos(), end_pos);

    			Iterator iter_end = ctr.seek(end_pos);

    			assertIteratorEQ(MA_SRC, iter, iter_end);
    		}
    	}
    }
};




}


#endif
