
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SYMBOLSEQ_SYMSEQ_MISC_TEST_HPP_
#define MEMORIA_TESTS_SYMBOLSEQ_SYMSEQ_MISC_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/tools/symbol_sequence.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <Int Bits>
class SymSeqMiscTest: public TestTask {

    typedef SymSeqMiscTest<Bits> MyType;

    typedef SymbolSequence<Bits> Seq;

    typedef typename Seq::Symbol Symbol;

    static const Int Symbols = Seq::Symbols;

public:

    SymSeqMiscTest(): TestTask((SBuf()<<"Misc."<<Bits).str())
    {
    	MEMORIA_ADD_TEST(testCreate);
    	MEMORIA_ADD_TEST(testAdapter);
    }

    virtual ~SymSeqMiscTest() throw() {}


    void testCreate()
    {
    	Seq seq;

    	size_t capacity = seq.capacity();
    	for (size_t c = 0; c < capacity; c++)
    	{
    		seq.pushBack(c);
    	}

    	AssertEQ(MA_SRC, seq.capacity(), 0ull);

    	Symbol mask = MakeMask<Symbol>(0, Bits);

    	for (size_t c = 0; c < seq.size(); c++)
    	{
    		AssertEQ(MA_SRC, seq[c].value(), c & mask);
    	}

    	size_t capacity2 = 16384;
    	Seq seq2(capacity2);

    	AssertEQ(MA_SRC, seq2.capacity(), capacity2);
    }

    void testAdapter()
    {
    	Seq seq1(4096);
    	Seq seq2(seq1.capacity());

    	seq1.resize(seq1.maxSize());
    	seq2.resize(seq2.maxSize());

    	size_t size = seq1.size();

    	for (Int c = 0; c < 1000; c++)
    	{
    		seq1.fillCells([](Symbol& cell) {
    			cell = getBIRandom();
    		});

    		AssertEQ(MA_SRC, seq1.capacity(), 0ull);

    		size_t src_start 	= getRandom(size);
    		size_t src_size 	= 1 + getRandom(size - src_start - 1);

    		size_t dst_start 	= getRandom(size - src_size);

    		auto adapter = seq1.source(src_start, src_size);

    		seq2.update(dst_start, adapter);

    		for (size_t c = src_start, dst_c = dst_start; c < src_start + src_size; c++, dst_c++)
    		{
    			Symbol src_value = seq1[c];
    			Symbol dst_value = seq2[dst_c];

    			AssertEQ(MA_SRC, src_value, dst_value, SBuf()<<c<<" "<<dst_c);
    		}
    	}
    }

};


}


#endif
