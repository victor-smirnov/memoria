
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SUM_SET_BATCH_SUM_SET_BATCH_TESTS_HPP_
#define MEMORIA_TESTS_SUM_SET_BATCH_SUM_SET_BATCH_TESTS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../shared/btree_test_base.hpp"
#include "params.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>


namespace memoria {

typedef SmallCtrTypeFactory::Factory<Set1>::Type SumSet1Ctr;

struct SumSetBatchReplay: public ReplayParams {

	Int 	data_;
	bool 	insert_;
	Int		block_size_;

	Int		page_step_;

	BigInt 	pos_;

	Int 	cnt_;

	SumSetBatchReplay(): ReplayParams(), data_(0), insert_(true), block_size_(0), page_step_(-1), pos_(-1), cnt_(0)
	{
		Add("data", 		data_);
		Add("insert", 		insert_);
		Add("block_size", 	block_size_);
		Add("page_step", 	page_step_);
		Add("pos", 			pos_);
		Add("cnt", 			cnt_);
	}
};




class SumSetBatchTest: public BTreeBatchTestBase<
	Set1,
	typename SumSet1Ctr::LeafPairsVector,
	SumSetBatchReplay
>
{
	typedef BTreeBatchTestBase<
			Set1,
			typename SumSet1Ctr::LeafPairsVector,
			SumSetBatchReplay
	>																Base;

	typedef typename Base::Ctr 										Ctr;
	typedef typename Base::Accumulator 								Accumulator;
	typedef typename SumSet1Ctr::LeafPairsVector 					ArrayData;

	static const Int Indexes 										= Ctr::Indexes;



public:
	SumSetBatchTest():
		Base("SumSetBatch")
	{
		size_ = 1024*1024;

		SmallCtrTypeFactory::Factory<Root>::Type::Init();
		Ctr::Init();
	}



	virtual ArrayData CreateBuffer(Ctr& ctr, Int size, UByte value)
	{
		ArrayData array(size);

		for (auto& pair: array)
		{
			pair.keys[0] = value;
		}

		return array;
	}

	virtual Iterator Seek(Ctr& array, BigInt pos)
	{
		Iterator i = array.Begin();

		for (BigInt c = 0; c < pos; c++)
		{
			if (!i.Next())
			{
				break;
			}
		}

		MEMORIA_TEST_THROW_IF(pos, !=, i.KeyNum());

		return i;
	}

	virtual void Insert(Iterator& iter, const ArrayData& data)
	{
		BigInt size = iter.model().GetSize();

		iter.model().InsertBatch(iter, data);

		MEMORIA_TEST_THROW_IF(size, ==, iter.model().GetSize());

		CheckSize(iter.model());
	}

	virtual void Read(Iterator& iter, ArrayData& data)
	{
		for (auto& value: data)
		{
			for (Int c = 0; c < Indexes; c++)
			{
				value.keys[c] = iter.GetRawKey(c);
			}

			if (!iter.Next())
			{
				break;
			}
		}
	}

	virtual void Remove(Iterator& iter, BigInt size)
	{
		auto iter2 = iter;
		Skip(iter2, size);
		Accumulator keys;
		iter.model().RemoveEntries(iter, iter2, keys);
	}

	virtual void Skip(Iterator& iter, BigInt offset)
	{
		if (offset > 0)
		{
			for (BigInt c = 0; c < offset; c++)
			{
				iter.Next();
			}
		}
		else {
			for (BigInt c = 0; c < -offset; c++)
			{
				iter.Prev();
			}
		}
	}

	virtual BigInt GetPosition(Iterator& iter)
	{
		return iter.KeyNum();
	}

	virtual BigInt GetLocalPosition(Iterator& iter)
	{
		return iter.key_idx();
	}

	virtual BigInt GetSize(Ctr& array)
	{
		return array.GetSize();
	}

	void CheckSize(Ctr& array)
	{
		BigInt cnt = 0;

		for (auto iter = array.Begin(); iter.IsNotEnd(); iter.Next())
		{
			cnt++;
		}

		MEMORIA_TEST_THROW_IF(cnt, !=, array.GetSize());
	}

	virtual void CheckIteratorPrefix(ostream& out, Iterator& iter, const char* source) {}

};

}


#endif

