
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_CXSEQ_HPP_
#define MEMORIA_TESTS_PALLOC_CXSEQ_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_dynamic_allocator.hpp>
#include <memoria/core/packed2/packed_fse_cxsequence.hpp>
#include <memoria/core/packed2/packed_wavelet_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedCxSequenceTest: public TestTask {

	typedef PackedCxSequenceTest 											MyType;


	typedef PackedFSECxSequenceTypes<8, char> 								Types;

	typedef PackedFSECxSequence<Types>										Sequence;

	typedef PackedSingleElementAllocator									Allocator;

	typedef shared_ptr<Allocator>											SequencePtr;

	typedef typename Sequence::SelectResult									SelectResult;

public:

	PackedCxSequenceTest(): TestTask("CxSeq")
    {
    	MEMORIA_ADD_TEST(testCreate);
    	MEMORIA_ADD_TEST(testRank);
    	MEMORIA_ADD_TEST(testSelect);
    }

    virtual ~PackedCxSequenceTest() throw() {}

    SequencePtr createSequence(Int size)
    {
    	Int sequence_block_size = Sequence::getBlockSize(64*1024);
    	Int block_size 			= Allocator::block_size(sequence_block_size) + 4096;

    	void* memory_block = malloc(block_size);
    	memset(memory_block, 0, block_size);

    	Allocator* allocator = T2T<Allocator*>(memory_block);

    	allocator->init(block_size);

    	AllocationBlock block = allocator->allocate(sequence_block_size);

    	Sequence* seq = T2T<Sequence*>(block.ptr());

    	seq->init(sequence_block_size);

    	seq->setAllocatorOffset(allocator);

    	return SequencePtr(allocator);
    }


    void testCreate()
    {
    	SequencePtr seq_ptr = createSequence(64*1024);
    	Sequence* seq = seq_ptr->get<Sequence>();

    	Int max_size = seq->max_size();
    	auto* symbols = seq->symbols();

    	for (Int c = 0; c < max_size; c++)
    	{
    		symbols[c] = getRandom(256);
    	}

    	seq->size() = max_size;

    	seq->reindex();

    	seq->dump();
    }

    SelectResult selectFW(const Sequence* seq, Int start, Int rank, Int symbol)
    {
    	Int total = 0;

    	const auto* buffer = seq->symbols();

    	for (Int c = start; c < seq->size(); c++)
    	{
    		total += buffer[c] == symbol;

    		if (total == rank)
    		{
    			return SelectResult(c, rank);
    		}
    	}

    	return SelectResult(seq->size(), total);
    }

    void testRank()
    {
    	SequencePtr seq_ptr = createSequence(64*1024);
    	Sequence* seq = seq_ptr->get<Sequence>();

    	Int max_size = seq->max_size();
    	auto* symbols = seq->symbols();

    	for (Int c = 0; c < max_size; c++)
    	{
    		symbols[c] = getRandom(256);
    	}

    	seq->size() = max_size;

    	seq->reindex();

    	Int rank = seq->rank(seq->size(), 0x11);

    	cout<<rank<<endl;
    	cout<<seq->select(rank/2 + 10, 0x11).idx()<<endl;
    	cout<<selectFW(seq, 0, rank/2 + 10, 0x11).idx()<<endl;
    }

    void testSelect()
    {
    	SequencePtr seq_ptr = createSequence(64*1024);
    	Sequence* seq = seq_ptr->get<Sequence>();

    	Int max_size = seq->max_size();
    	auto* symbols = seq->symbols();

    	for (Int c = 0; c < max_size; c++)
    	{
    		symbols[c] = getRandom(256);
    	}

    	seq->size() = max_size;

    	seq->reindex();

    	Int total_rank = seq->rank(seq->size(), 0x11);

//    	cout<<rank<<endl;
//    	cout<<seq->select(rank/2 + 10, 0x11).idx()<<endl;
//    	cout<<selectFW(seq, 0, rank/2 + 10, 0x11).idx()<<endl;

    	BigInt t0 = getTimeInMillis();

    	BigInt ops = 0;

    	for (Int c = 0; c < 1000; c++)
    	{
    		for (int rank = 1; rank <= total_rank; rank++, ops++)
    		{
    			seq->select(rank, 0x11);
    		}
    	}

    	BigInt t1 = getTimeInMillis();

    	cout<<"time: "<<FormatTime(t1 - t0)<<" ops="<<ops<<endl;
    }


};


}


#endif
