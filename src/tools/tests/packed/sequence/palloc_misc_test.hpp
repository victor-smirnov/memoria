
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_MISC_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_MISC_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "palloc_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <
	Int Bits,
	template <typename>	class IndexType 	= PkdFTree,
	template <typename>	class CodecType 	= ValueFSECodec,
	template <typename>	class ReindexFnType = BitmapReindexFn,
	template <typename>	class SelectFnType	= BitmapSelectFn,
	template <typename>	class RankFnType	= BitmapRankFn,
	template <typename>	class ToolsFnType	= BitmapToolsFn
>
class PackedSearchableSequenceMiscTest: public PackedSearchableSequenceTestBase<
	Bits,
	IndexType,
	CodecType,
	ReindexFnType,
	SelectFnType,
	RankFnType,
	ToolsFnType
> {

    typedef PackedSearchableSequenceMiscTest<
    		Bits,
    		IndexType,
    		CodecType,
    		ReindexFnType,
    		SelectFnType,
    		RankFnType,
    		ToolsFnType
    > 																			MyType;

    typedef PackedSearchableSequenceTestBase<
    		Bits,
    		IndexType,
    		CodecType,
    		ReindexFnType,
    		SelectFnType,
    		RankFnType,
    		ToolsFnType
    > 																			Base;

    typedef typename Base::Seq													Seq;

    typedef typename Seq::Value													Value;


    static const Int Blocks                 = Seq::Indexes;
    static const Int Symbols                = 1<<Bits;
    static const Int VPB					= Seq::ValuesPerBranch;

public:

    PackedSearchableSequenceMiscTest(StringRef name): Base(name)
    {
    	MEMORIA_ADD_TEST(testCreate);
    	MEMORIA_ADD_TEST(testInsertSingle);
    	MEMORIA_ADD_TEST(testInsertMultiple);
    	MEMORIA_ADD_TEST(testRemoveMulti);
    	MEMORIA_ADD_TEST(testRemoveAll);
    	MEMORIA_ADD_TEST(testClear);
    }

    virtual ~PackedSearchableSequenceMiscTest() throw() {}

    void testCreate()
    {
    	for (Int size = 1; size < this->size_; size *= 2)
    	{
    		this->out()<<size<<std::endl;

    		Seq* seq = this->createEmptySequence();
    		PARemover remover(seq);

    		auto symbols = this->fillRandom(seq, size);

    		this->assertIndexCorrect(MA_SRC, seq);
    		this->assertEqual(seq, symbols);

//    		seq->dump(this->out());
    	}
    }


    void testInsertSingle()
    {
    	for (Int size = 1; size <= this->size_; size *= 2)
    	{
    		this->out()<<size<<std::endl;

    		Seq* seq = this->createEmptySequence();
    		PARemover remover(seq);

    		auto symbols = this->fillRandom(seq, size);

    		for (Int c = 0; c < this->iterations_; c++)
    		{
    			Int idx 	= getRandom(seq->size());
    			Int symbol 	= getRandom(Blocks);

    			seq->insert(idx, symbol);

    			symbols.insert(symbols.begin() + idx, symbol);

    			this->assertIndexCorrect(MA_SRC, seq);
    			this->assertEqual(seq, symbols);
    		}
    	}
    }

    void testInsertMultiple()
    {
    	for (Int size = 1; size <= this->size_; size *= 2)
    	{
    		this->out()<<size<<std::endl;

    		Seq* seq = this->createEmptySequence();
    		PARemover remover(seq);

    		auto symbols = this->fillRandom(seq, size);

    		for (Int c = 0; c < this->iterations_; c++)
    		{
    			Int idx 	= getRandom(seq->size());

    			vector<Int> block(10);
    			for (Int d = 0; d < block.size(); d++)
    			{
    				block[d] = getRandom(Blocks);
    			}

    			Int cnt = 0;
    			seq->insert(idx, block.size(), [&](){
    				return block[cnt++];
    			});

    			symbols.insert(symbols.begin() + idx, block.begin(), block.end());

    			this->assertIndexCorrect(MA_SRC, seq);
    			this->assertEqual(seq, symbols);
    		}
    	}
    }

    void testRemoveMulti()
    {
    	for (Int size = 1; size <= this->size_; size *= 2)
    	{
    		this->out()<<size<<std::endl;

    		Seq* seq = this->createEmptySequence();
    		PARemover remover(seq);

    		auto symbols = this->fillRandom(seq, size);

    		for (Int c = 0; c < this->iterations_; c++)
    		{
    			Int start 	= getRandom(seq->size());
    			Int end 	= start + getRandom(seq->size() - start);

    			Int block_size = seq->block_size();

    			seq->remove(start, end);

    			symbols.erase(symbols.begin() + start, symbols.begin() + end);

    			this->assertIndexCorrect(MA_SRC, seq);
    			this->assertEqual(seq, symbols);

    			AssertLE(MA_SRC, seq->block_size(), block_size);
    		}
    	}
    }

    void testRemoveAll()
    {
    	for (Int size = 1; size <= this->size_; size *= 2)
    	{
    		this->out()<<size<<std::endl;

    		Seq* seq = this->createEmptySequence();
    		PARemover remover(seq);

    		auto symbols = this->fillRandom(seq, size);
    		this->assertEqual(seq, symbols);

    		seq->remove(0, seq->size());

    		this->assertEmpty(seq);
    	}
    }

    void testClear()
    {
    	for (Int size = 1; size <= this->size_; size *= 2)
    	{
    		this->out()<<size<<std::endl;

    		Seq* seq = this->createEmptySequence();
    		PARemover remover(seq);

    		this->assertEmpty(seq);

    		this->fillRandom(seq, size);

    		AssertNEQ(MA_SRC, seq->size(), 0);
    		AssertGT(MA_SRC, seq->block_size(), Seq::empty_size());

    		seq->clear();

    		this->assertEmpty(seq);
    	}
    }

};


}


#endif
