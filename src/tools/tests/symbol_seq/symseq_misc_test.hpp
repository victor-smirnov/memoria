
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SYMBOLSEQ_SYMSEQ_MISC_TEST_HPP_
#define MEMORIA_TESTS_SYMBOLSEQ_SYMSEQ_MISC_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed/wrappers/symbol_sequence.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <Int Bits>
class SymSeqMiscTest: public TestTask {

    typedef SymSeqMiscTest<Bits> MyType;

    typedef PackedFSESequence<Bits> Seq;

    typedef typename Seq::Symbol Symbol;

    static const Int Symbols = Seq::Symbols;

    Int iterations_ = 1000;

public:

    SymSeqMiscTest(): TestTask((SBuf()<<"Misc."<<Bits).str())
    {
    	size_ = 10000;

    	MEMORIA_ADD_TEST_PARAM(iterations_);

    	MEMORIA_ADD_TEST(testCreate);
    	MEMORIA_ADD_TEST(testAdapter);
    }

    virtual ~SymSeqMiscTest() throw() {}


    void testCreate()
    {
    	Seq seq;

    	for (Int c = 0; c < this->size_; c++)
    	{
    		seq.append(c);
    	}

    	AssertEQ(MA_SRC, seq.size(), this->size_);

    	Symbol mask = MakeMask<Symbol>(0, Bits);

    	for (Int c = 0; c < seq.size(); c++)
    	{
    		AssertEQ(MA_SRC, seq[c].value(), c & mask);
    	}
    }

    void testAdapter()
    {
    	Seq seq1;

    	seq1.append(this->size_, []() -> Symbol {
    		return getRandom(Symbols);
    	});

    	AssertEQ(MA_SRC, seq1.size(), this->size_);

    	Int size = this->size_;

    	for (Int c = 0; c < iterations_; c++)
    	{
    		Seq seq2;

    		Int src_start 	= getRandom(size);
    		Int src_size 	= 1 + getRandom(size - src_start - 1);

    		Int dst_start 	= getRandom(size - src_size);

    		auto adapter = seq1.source(src_start, src_size);

    		seq2.update(dst_start, adapter);

    		for (Int c = src_start, dst_c = dst_start; c < src_start + src_size; c++, dst_c++)
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
