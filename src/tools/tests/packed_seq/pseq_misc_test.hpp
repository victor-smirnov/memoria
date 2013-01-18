
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
    typedef typename Seq::IndexKey				IndexKey;

    typedef vector<UShort>						Buffer;

    static const Int Symbols					= Seq::Symbols;

public:

    PSeqMiscTest(): TestTask("Misc")
    {
        MEMORIA_ADD_TEST(testInit);

        MEMORIA_ADD_TEST(testTransferTo);
        MEMORIA_ADD_TEST(testResize);
        MEMORIA_ADD_TEST(testCopyTo);
        MEMORIA_ADD_TEST(testRemoveSpace);
        MEMORIA_ADD_TEST(testInsertSpace);
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

    Seq* createEmptySequence(Int buffer_size, Int init_size = -1) const
    {
    	if (init_size == -1) init_size = buffer_size;

    	Byte* buffer       	= new Byte[buffer_size];

    	memset(buffer, 0, buffer_size);

    	Seq* seq = T2T<Seq*>(buffer);
    	seq->initByBlock(init_size - sizeof(Seq));

    	return seq;
    }


    Buffer populateRandom(Seq* seq)
    {
    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Seq::Symbols);
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();

    	Buffer data(seq->size());

    	for (Int c = 0; c < seq->size(); c++)
    	{
    		data[c] = seq->value(c);
    	}

    	return data;
    }

    void compare(const Seq* seq, const Buffer& data)
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

    	Buffer data = populateRandom(seq1);

    	seq1->transferTo(seq2);

    	seq2->size() = seq1->size();

    	seq2->reindex();

    	compare(seq1, seq2);

    	Seq* seq3 = createEmptySequence(1024);

    	AssertThrows<Exception>(MA_SRC, [&](){
    		seq1->transferTo(seq3);
    	});
    }

    void testResize(ostream& out)
    {
    	Seq* seq1 = createEmptySequence(8192, 2048);
    	Buffer data = populateRandom(seq1);

    	IndexKey sums[Symbols];

    	for (Int c = 0; c < Symbols; c++) sums[c] = seq1->maxIndex(c);

    	seq1->enlargeBlock(8192);

    	seq1->reindex();

    	for (Int c = 0; c < Symbols; c++)
    	{
    		AssertEQ(MA_SRC, sums[c], seq1->maxIndex(c));
    	}

    	compare(seq1, data);

    	seq1->shrinkBlock(2048);

    	seq1->reindex();

    	for (Int c = 0; c < Symbols; c++)
    	{
    		AssertEQ(MA_SRC, sums[c], seq1->maxIndex(c));
    	}

    	compare(seq1, data);
    }

    void copyTo(const Buffer& src, Buffer& dst, Int src_idx, Int dst_idx, Int length)
    {
    	if (dst_idx > src_idx)
    	{
    		for (Int c = src_idx + length - 1; c >= src_idx; c--)
    		{
    			dst[c - src_idx + dst_idx] = src[c];
    		}
    	}
    	else {
    		for (Int c = src_idx; c < src_idx + length; c++)
    		{
    			dst[c - src_idx + dst_idx] = src[c];
    		}
    	}
    }

    void assertCopyTo(
    		const Seq* src,
    		Seq* dst,
    		const Buffer& src_data,
    		Buffer& dst_data,
    		Int src_idx,
    		Int dst_idx,
    		Int length
    )
    {
    	src->copyTo(dst, src_idx, length, dst_idx);

    	copyTo(src_data, dst_data, src_idx, dst_idx, length);

    	compare(dst, dst_data);
    }

    void testCopyTo(ostream& out)
    {
    	Seq* seq1 = createEmptySequence(1024);
    	Seq* seq2 = createEmptySequence(1024);

    	Buffer data1 = populateRandom(seq1);
    	Buffer data2 = populateRandom(seq2);

    	for (Int c = 0; c < 100; c++)
    	{
    		out<<c<<endl;

    		Int src_idx = getRandom(seq1->size()) / 4 * 3;
    		Int dst_idx;
    		Int length;

    		if (getRandom(2))
    		{
    			dst_idx = src_idx + getRandom(seq1->size() - src_idx);
    			length  = 1 + getRandom(seq1->size() - dst_idx - 1);
    		}
    		else
    		{
    			dst_idx = getRandom(src_idx);
    			length  = 1 + getRandom(seq1->size() - src_idx - 1);
    		}

    		assertCopyTo(seq1, seq2, data1, data2, src_idx, dst_idx, length);
    		assertCopyTo(seq2, seq1, data2, data1, src_idx, dst_idx, length);

    		assertCopyTo(seq1, seq1, data1, data1, src_idx, dst_idx, length);
    	}
    }

    void removeSpace(Buffer& data, Int idx, Int length)
    {
    	data.erase(data.begin() + idx, data.begin() + idx + length);
    }

    void testRemoveSpace(ostream& out)
    {
    	Seq* seq = createEmptySequence(8192);
    	Buffer data = populateRandom(seq);

    	while (seq->size() > 0)
    	{
    		out<<seq->size()<<endl;

    		Int idx 		= getRandom(seq->size());
    		Int max_length 	= seq->size() - idx - 1;
    		Int length		= 1 + getRandom(max_length < 200 ? max_length : 200);

    		seq->removeSpace(idx, length);
    		removeSpace(data, idx, length);

    		compare(seq, data);
    	}
    }

    void testInsertSpace(ostream& out)
    {
    	Seq* seq = createEmptySequence(8192);
    	Buffer data;

    	while (seq->capacity() > 0)
    	{
    		out<<seq->size()<<endl;

    		Int idx 		= getRandom(seq->size());
    		Int max_length 	= seq->capacity();
    		Int length		= 1 + getRandom(max_length < 200 ? max_length: 200);

    		seq->insertSpace(idx, length);

    		for (Int c = idx; c < idx + length; c++)
    		{
    			IndexKey value = getRandom(Symbols);
    			seq->value(c)  = value;

    			data.insert(data.begin() + c, value);
    		}

    		compare(seq, data);
    	}
    }

};


}


#endif
