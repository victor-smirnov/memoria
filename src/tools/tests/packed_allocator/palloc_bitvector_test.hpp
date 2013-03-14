
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_BITVECTOR_HPP_
#define MEMORIA_TESTS_PALLOC_BITVECTOR_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_allocator.hpp>
#include <memoria/core/packed2/packed_bitvector.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedBitVectorTest: public TestTask {

	typedef TestTask														Base;
	typedef PackedBitVectorTest 											MyType;


	typedef PackedAllocator													Allocator;

	typedef PackedBitVectorTypes<> 											Types;

	typedef PackedBitVector<Types>											Sequence;

	typedef shared_ptr<Allocator>											SequencePtr;

	typedef typename Sequence::SelectResult									SelectResult;

public:

	PackedBitVectorTest(): TestTask("BitVector")
    {
    	MEMORIA_ADD_TEST(testCreate);
    	MEMORIA_ADD_TEST(testRank);
    	MEMORIA_ADD_TEST(testSelect);
    }

    virtual ~PackedBitVectorTest() throw() {}

    SequencePtr createSequence(Int size)
    {
    	Int sequence_block_size = Sequence::block_size(size);
    	Int block_size 			= Allocator::block_size(sequence_block_size, 1) + 4096;

    	void* memory_block = malloc(block_size);
    	memset(memory_block, 0, block_size);

    	Allocator* allocator = T2T<Allocator*>(memory_block);

    	allocator->init(block_size, 1);

    	AllocationBlock block = allocator->allocate(0, sequence_block_size);

    	Sequence* seq = T2T<Sequence*>(block.ptr());

    	seq->init(sequence_block_size);

    	seq->setAllocatorOffset(allocator);

    	return SequencePtr(allocator);
    }


    void testCreate()
    {
    	SequencePtr seq_ptr = createSequence(64*1024);
    	Sequence* seq = seq_ptr->get<Sequence>(0);

    	Int max_size = seq->max_size();

    	for (Int c = 0; c < max_size; c++)
    	{
    		seq->value(c) = getRandom(2);
    	}

    	seq->size() = max_size;

    	seq->reindex();

    	seq->dump(out());
    }

    SelectResult selectFW(const Sequence* seq, Int start, Int rank, Int symbol)
    {
    	Int total = 0;

    	for (Int c = start; c < seq->size(); c++)
    	{
    		total += seq->value(c) == symbol;

    		if (total == rank)
    		{
    			return SelectResult(c, rank, true);
    		}
    	}

    	return SelectResult(seq->size(), total, false);
    }

    void testRank()
    {
    	SequencePtr seq_ptr = createSequence(2048);
    	Sequence* seq = seq_ptr->get<Sequence>(0);

    	Int max_size = seq->max_size();

    	for (Int c = 0; c < max_size; c++)
    	{
    		seq->value(c) = getRandom(2);
    	}

    	seq->size() = max_size;

    	seq->reindex();

//    	seq->dump();

    	Int rank = seq->rank(seq->size(), 1);
    	cout<<rank<<endl;

    	cout<<seq->select(rank - 10, 1).idx()<<endl;
    	cout<<selectFW(seq, 0, rank - 10, 1).idx()<<endl;
    }

    void testSelect()
    {
    	SequencePtr seq_ptr = createSequence(64*1024);
    	Sequence* seq = seq_ptr->get<Sequence>(0);

    	Int max_size = seq->max_size();


    	for (Int c = 0; c < max_size; c++)
    	{
    		seq->value(c) = getRandom(2);
    	}

    	seq->size() = max_size;

    	seq->reindex();

    	Int total_rank = seq->rank(seq->size(), 1);

    	BigInt t0 = getTimeInMillis();

    	BigInt ops = 0;

    	for (int rank = 1; rank <= total_rank; rank++, ops++)
    	{
    		auto result1 = seq->select(rank, 1);
    		auto result2 = selectFW(seq, 0, rank, 1);

    		AssertEQ(MA_SRC, result1.idx(), result2.idx());
    		AssertEQ(MA_SRC, result1.rank(), result2.rank());

    		Int rnk = seq->rank(result1.idx() + 1, 1);

    		AssertEQ(MA_SRC, rnk, rank);
    	}

    	BigInt t1 = getTimeInMillis();

    	cout<<"time: "<<FormatTime(t1 - t0)<<" ops="<<ops<<endl;
    }


};


}


#endif
