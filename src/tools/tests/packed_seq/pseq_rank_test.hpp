
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQ_PSEQ_RANK_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQ_PSEQ_RANK_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_seq.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <Int Bits>
class PSeqRankTest: public TestTask {

    typedef PSeqRankTest MyType;

    typedef PackedSeqTypes<
    		UInt,
    		UBigInt,
    		Bits,
    		PackedSeqBranchingFactor,
    		PackedSeqValuesPerBranch / (Bits * 2)
    > 																			Types;


    typedef typename Types::IndexKey        Key;
    typedef typename Types::Value           Value;

    typedef PackedSeq<Types>            	Seq;

    static const Int Blocks                 = Seq::Blocks;
    static const Int Symbols                = 1<<Seq::Bits;

public:

    PSeqRankTest(): TestTask((SBuf()<<"Rank."<<Bits).str())
    {
        MEMORIA_ADD_TEST(runTest1);
        MEMORIA_ADD_TEST(runTest2);
        MEMORIA_ADD_TEST(runTest3);
    }

    virtual ~PSeqRankTest() throw() {}


    void runTest1(ostream& out)
    {
    	out<<"runTest1: "<<Bits<<endl;

    	Int buffer_size     = 2048*Bits;

    	unique_ptr<Byte[]>  buffer_ptr(new Byte[buffer_size]);
    	Byte* buffer       	= buffer_ptr.get();
    	memset(buffer, 0, buffer_size);

    	Seq* seq           = T2T<Seq*>(buffer);
    	seq->initByBlock(buffer_size - sizeof(Seq));

    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Blocks == 1 ? 2 : Blocks);
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();

    	for (Int start = 0; start < seq->maxSize(); start += 10)
    	{
    		out<<start<<endl;
    		for (Int end = start; end < seq->maxSize(); end += 10)
    		{
    			for (Int s = 0; s < Symbols; s++)
    			{
    				Int rank = seq->rank(start, end, s);
    				Int popc = seq->popCount(start, end, s);

    				AssertEQ(MA_SRC, rank, popc);
    			}
    		}
    	}
    }

    void runTest2(ostream& out)
    {
    	out<<"runTest2: "<<Bits<<endl;

    	Int buffer_size     = 4096*Bits;

    	unique_ptr<Byte[]>  buffer_ptr(new Byte[buffer_size]);
    	Byte* buffer       	= buffer_ptr.get();
    	memset(buffer, 0, buffer_size);

    	Seq* seq           = T2T<Seq*>(buffer);
    	seq->initByBlock(buffer_size - sizeof(Seq));

    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Blocks == 1 ? 2 : Blocks);
    	}

    	for (Int end = 0; end < seq->maxSize(); end++)
    	{
    		seq->size() = end;
    		seq->reindex();

    		for (Int s = 0; s < Symbols; s++)
    		{
    			Int rank = seq->rank(0, end, s);
    			Int popc = seq->popCount(0, end, s);

    			AssertEQ(MA_SRC, rank, popc, SBuf()<<end<<" "<<s);
    		}
    	}
    }

    void runTest3(ostream& out)
    {
    	out<<"runTest3: "<<Bits<<endl;

    	Int buffer_size     = 4096*Bits;

    	unique_ptr<Byte[]>  buffer_ptr(new Byte[buffer_size]);
    	Byte* buffer       	= buffer_ptr.get();
    	memset(buffer, 0, buffer_size);

    	Seq* seq           = T2T<Seq*>(buffer);
    	seq->initByBlock(buffer_size - sizeof(Seq));

    	for (Int c = 0; c < seq->maxSize(); c++)
    	{
    		seq->value(c) = getRandom(Blocks == 1 ? 2 : Blocks);
    	}

    	seq->size() = seq->maxSize();
    	seq->reindex();

    	Int end = seq->size();

    	for (Int start = 0; start < seq->maxSize(); start++)
    	{
    		for (Int s = 0; s < Symbols; s++)
    		{
    			Int rank = seq->rank(start, end, s);
    			Int popc = seq->popCount(start, end, s);

    			AssertEQ(MA_SRC, rank, popc);
    		}
    	}
    }
};


}


#endif
