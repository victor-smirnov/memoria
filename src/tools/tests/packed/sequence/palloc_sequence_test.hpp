
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_SEQUENCE_TEST_HPP_
#define MEMORIA_TESTS_PALLOC_SEQUENCE_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed2/packed_fse_searchable_seq.hpp>

#include <memory>

namespace memoria {

template <Int BitsPerSymbol>
class PackedSearchableSequenceTest: public TestTask {

	typedef PackedSearchableSequenceTest 										MyType;


	typedef PackedFSESeachableSeqTypes<BitsPerSymbol> 							Types;

	typedef PackedFSESearchableSeq<Types>										Sequence;

	typedef typename Sequence::Value											Value;


public:

	PackedSearchableSequenceTest(): TestTask("Seq")
    {
    	MEMORIA_ADD_TEST(testCreate);
    }

    virtual ~PackedSearchableSequenceTest() throw() {}


    Int rank(const Sequence* seq, Int start, Int end, Int symbol)
    {
    	Int rank = 0;

    	for (Int c = start; c < end; c++)
    	{
    		rank += seq->test(c, symbol);
    	}

    	return rank;
    }

    SelectResult selectFw(const Sequence* seq, Int start, Int symbol, Int rank)
    {
    	Int total = 0;

    	for (Int c = start; c < seq->size(); c++)
    	{
    		total += seq->test(c, symbol);

    		if (total == rank)
    		{
    			return SelectResult(c, rank, true);
    		}
    	}

    	return SelectResult(seq->size(), total, false);
    }

    Sequence* createSequence(Int capacity)
    {
    	Int sequence_block_size = Sequence::empty_size() +
    			PackedAllocator::roundUpBitToBytes(capacity * BitsPerSymbol) * 150 / 100;

    	Int block_size = PackedAllocator::block_size(sequence_block_size, 1);

    	void* memory_block = malloc(block_size);
    	memset(memory_block, 0, block_size);

    	PackedAllocator* allocator = T2T<PackedAllocator*>(memory_block);

    	allocator->init(block_size, 1);

    	allocator->allocateEmpty<Sequence>(0);

    	Sequence* seq = allocator->template get<Sequence>(0);

    	return seq;
    }

    void fillRandom(Sequence* seq, Int size) {
    	seq->insert(0, size, [](){
    		return getRandom(1 << BitsPerSymbol);
    	});
    }

    void testCreate()
    {
    	Int size = 32768;

    	Sequence* seq= createSequence(size);

    	fillRandom(seq, size);

    	free(seq->allocator());
    }

    void testRank()
    {
    	Int size = 32768;

    	Sequence* seq= createSequence(size);

    	fillRandom(seq, size);


    }
};


}


#endif
