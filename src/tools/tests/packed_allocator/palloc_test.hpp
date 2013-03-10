
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_INIT_HPP_
#define MEMORIA_TESTS_PALLOC_INIT_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_dynamic_allocator.hpp>
#include <memoria/core/packed2/packed_fse_cxsequence.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedAllocatorTest: public TestTask {

	typedef PackedAllocatorTest 											MyType;


	typedef PackedFSECxSequenceTypes<8, char> 								Types;

	typedef PackedFSECxSequence<Types>										Sequence;

	typedef PackedSingleElementAllocator<Sequence>							Allocator;

	typedef shared_ptr<Allocator>											SequencePtr;

public:

	PackedAllocatorTest(): TestTask("Test")
    {
    	MEMORIA_ADD_TEST(testCreate);
    }

    virtual ~PackedAllocatorTest() throw() {}

    SequencePtr createSequence(Int size)
    {
    	Int sequence_block_size = Sequence::getBlockSize(64*1024);
    	Int block_size 			= Allocator::block_size(sequence_block_size) + 4096;

    	void* memory_block = malloc(block_size);
    	memset(memory_block, 0, block_size);

    	Allocator* allocator = T2T<Allocator*>(memory_block);

    	allocator->initBlockSize(block_size);

    	AllocationBlock block = allocator->allocate(sequence_block_size);

    	Sequence* seq = T2T<Sequence*>(block.ptr());

    	seq->initBlockSize(sequence_block_size);

    	seq->setAllocatorOffset(allocator);

    	return SequencePtr(allocator);
    }


    void testCreate()
    {
    	SequencePtr seq_ptr = createSequence(64*1024);
    	Sequence* seq = seq_ptr->get();

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


};


}


#endif
