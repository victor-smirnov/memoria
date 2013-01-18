
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQ_PSEQ_MISC_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQ_PSEQ_MISC_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_seq.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <Int Bits>
class PSeqMiscTest: public TestTask {

    typedef PSeqMiscTest<Bits> MyType;

    typedef PackedSeqTypes<Int, UBigInt, Bits>  Types;
    typedef PackedSeq<Types>					Seq;
    typedef typename Seq::Value					Value;

public:

    PSeqMiscTest(): TestTask("Misc")
    {
        MEMORIA_ADD_TEST(testInit);

        MEMORIA_ADD_TEST(testTransferTo);
    }

    virtual ~PSeqMiscTest() throw() {}

    void testInit(ostream& out)
    {
    	Seq seq;

    	for (Int block_size = 512 * Bits; block_size < 100000; block_size++)
    	{
    		seq.initByBlock(block_size);

    		AssertLE(MA_SRC, seq.getBlockSize(), block_size);
    	}
    }

    Seq* createEmptySequence(Int buffer_size) const
    {
    	Byte* buffer       	= new Byte[buffer_size];

    	memset(buffer, 0, buffer_size);

    	Seq* seq = T2T<Seq*>(buffer);
    	seq->initByBlock(buffer_size - sizeof(Seq));

    	return seq;
    }


    vector<UByte> populateRandom(Seq* seq)
    {
    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Seq::Symbols);
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();

    	vector<UByte> data(seq->size());

    	for (Int c = 0; c < seq->size(); c++)
    	{
    		data[c] = seq->value(c);
    	}

    	return data;
    }

    void compare(const Seq* seq, const vector<UByte>& data)
    {
    	AssertEQ(MA_SRC, seq->size(), (Int)data.size());

    	for (Int c = 0; c < seq->size(); c++)
    	{
    		AssertEQ(MA_SRC, seq->value(c), data[c], SBuf()<<"c="<<c);
    	}
    }

    void compare(const Seq* src, const Seq* tgt)
    {
    	AssertEQ(MA_SRC, src->size(), tgt->size());

    	for (Int c = 0; c < src->size(); c++)
    	{
    		AssertEQ(MA_SRC, src->value(c), tgt->value(c), SBuf()<<"c="<<c);
    	}

    	for (Int s = 0; s < Seq::Symbols; s++)
    	{
    		for (Int c = 0; c < src->indexSize(); c++)
    		{
    			AssertEQ(MA_SRC, src->index(s, c), src->index(s, c), SBuf()<<"(s, c)=("<<s<<", "<<c<<")");
    		}
    	}
    }

    void testTransferTo(ostream& out)
    {
    	Seq* seq1 = createEmptySequence(8192);
    	Seq* seq2 = createEmptySequence(8192);

    	vector<UByte> data = populateRandom(seq1);

    	seq1->transferTo(seq2);

    	seq2->size() = seq1->size();

    	seq2->reindex();

    	compare(seq1, seq2);

    	Seq* seq3 = createEmptySequence(1024);

    	AssertThrows<Exception>(MA_SRC, [&](){
    		seq1->transferTo(seq3);
    	});
    }

};


}


#endif
