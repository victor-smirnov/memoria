
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_ITER_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_ITER_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceIteratorTest: public SPTestTask{

    typedef SequenceIteratorTest<BitsPerSymbol, Dense>                          MyType;
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
    SequenceIteratorTest(StringRef name):
        Base(name)
    {
        Base::size_ = 2048*1024;

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();

        MEMORIA_ADD_TEST(runIteratorTest);
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


    void runIteratorTest(ostream& out)
    {
    	DefaultLogHandlerImpl logHandler(out);
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
};




}


#endif
