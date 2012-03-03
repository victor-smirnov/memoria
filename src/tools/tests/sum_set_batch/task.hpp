
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_IDX_SET_BATCH_TESTS_HPP_
#define MEMORIA_TESTS_IDX_SET_BATCH_TESTS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>


#include "params.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>


namespace memoria {

class SumSetBatchTest: public SPTestTask {

private:
	typedef vector<BigInt> PairVector;
	typedef StreamContainerTypesCollection::Factory<SumSet1>::Type 	SumSetCtr;
	typedef SumSetCtr::Iterator										Iterator;

	static const Int Indexes = SumSetCtr::Indexes;
	typedef typename SumSetCtr::Key Key;

	typedef typename SumSetCtr::Counters 					Counters;
	typedef typename SumSetCtr::NodeBaseG 					NodeBaseG;
	typedef typename SumSetCtr::NodeDispatcher 				NodeDispatcher;
	typedef typename SumSetCtr::NonLeafDispatcher 			NonLeafDispatcher;
	typedef typename SumSetCtr::LeafDispatcher 				LeafDispatcher;
	typedef typename SumSetCtr::ID 							ID;
	typedef typename SumSetCtr::LeafNodeKeyValuePair 		LeafNodeKeyValuePair;
	typedef typename SumSetCtr::NonLeafNodeKeyValuePair 	NonLeafNodeKeyValuePair;
	typedef typename SumSetCtr::LeafPairsVector				LeafPairsVector;
	typedef typename SumSetCtr::LeafPairsVector				ArrayData;

	PairVector pairs;
	PairVector pairs_sorted;


public:

	SumSetBatchTest(): SPTestTask(new SumSetBatchParams()) 	{}
	virtual ~SumSetBatchTest() throw() 						{}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new SumSetBatchReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		SumSetBatchReplay* params = static_cast<SumSetBatchReplay*>(step_params);
		Allocator allocator;
		LoadAllocator(allocator, params);
		SumSetCtr dv(allocator, 1);

		dv.SetMaxChildrenPerNode(params->btree_airity_);

		if (params->insert_)
		{
			Build(out, allocator, dv, params);
		}
		else {
			Remove(allocator, dv, params);
		}
	}

	BigInt GetRandomPosition(SumSetCtr& array)
	{
		BigInt size = array.GetSize();
		return GetBIRandom(size);
	}


	virtual void Run(ostream& out)
	{
		SumSetBatchReplay params;
		SumSetBatchParams* task_params = GetParameters<SumSetBatchParams>();

		if (task_params->btree_random_airity_)
		{
			task_params->btree_airity_ = 8 + GetRandom(100);
			out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
		}

		params.size_			= task_params->size_;
		params.btree_airity_ 	= task_params->btree_airity_;

		for (Int step = 0; step < 2; step++)
		{
			params.step_ = step;
			Run(out, params, task_params, false);
		}

		// Run() will use different step for each ByteArray update operation
		Run(out, params, task_params, true);
	}

	void Run(ostream& out, SumSetBatchReplay& params, SumSetBatchParams* task_params, bool step)
	{
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);
		SumSetCtr dv(allocator, 1, true);

		allocator.commit();

		dv.SetMaxChildrenPerNode(params.btree_airity_);

		try {
			out<<"Insert data"<<endl;
			params.insert_ = true;


//			Int cnt = 0;

			params.data_ = 1;
			while (dv.GetSize() < params.size_)
			{
				if (step)
				{
					params.step_ 		= GetRandom(3);
				}

				params.data_size_ 	= 1 + GetRandom(task_params->max_block_size_);

				Build(out, allocator, dv, &params);

				allocator.commit();

//				StoreAllocator(allocator, "alloc" + ToString(cnt++)+".dump");

				params.data_++;

				params.pos_ 		= -1;
				params.page_step_ 	= -1;
			}

			//StoreAllocator(allocator, "allocator.dump");

			out<<"Remove data. SumSet contains "<<(dv.GetSize()/1024)<<"K keys"<<endl;
			params.insert_ = false;

			for (Int c = 0; ; c++)
			{
				if (step)
				{
					params.step_ = GetRandom(3);
				}

				BigInt size = dv.GetSize();
				BigInt max_size = task_params->max_block_size_ <= size ? task_params->max_block_size_ : size;

				params.data_size_ = 1 + GetBIRandom(max_size);
				params.page_step_ 	= GetRandom(3);

				if (!Remove(allocator, dv, &params))
				{
					break;
				}

				params.pos_ 		= -1;
				params.page_step_ 	= -1;

				allocator.commit();
			}

			out<<"SumSet.size = "<<(dv.GetSize() / 1024)<<"K keys"<<endl;

			allocator.commit();
		}
		catch (...)
		{
			Store(allocator, &params);
			throw;
		}
	}

	LeafPairsVector CreateBuffer(Int size, UByte value)
	{
		LeafPairsVector array(size);

		for (LeafNodeKeyValuePair& pair: array)
		{
			pair.keys[0] = value;
		}

		return array;
	}

	Iterator Seek(SumSetCtr& array, BigInt pos)
	{
		Iterator i = array.Begin();

		for (BigInt c = 0; c < pos; c++)
		{
			if (!i.Next())
			{
				break;
			}
		}

		MEMORIA_TEST_ASSERT(pos, !=, i.KeyNum());

		return i;
	}

	void Insert(Iterator& iter, const LeafPairsVector& data)
	{
		BigInt size = iter.model().GetSize();

		iter.model().InsertBatch(iter, data);

		MEMORIA_TEST_ASSERT(size, ==, iter.model().GetSize());

		CheckSize(iter.model());
	}

	void Read(Iterator& iter, LeafPairsVector& data)
	{
		for (LeafNodeKeyValuePair& value: data)
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

	void Skip(Iterator& iter, BigInt offset)
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

	void CheckSize(SumSetCtr& array)
	{
		BigInt cnt = 0;

		for (auto iter = array.Begin(); iter.IsNotEnd(); iter.Next())
		{
			cnt++;
		}

		MEMORIA_TEST_ASSERT(cnt, !=, array.GetSize());
	}


	void Build(ostream& out, Allocator& allocator, SumSetCtr& array, SumSetBatchReplay *params)
	{
		UByte value = params->data_;
		Int step 	= params->step_;

		LeafPairsVector data = CreateBuffer(params->data_size_, value);

		BigInt size = array.GetSize();

		if (size == 0)
		{
			//Insert buffer into an empty array
			auto iter = Seek(array, 0);

			Insert(iter, data);

			Check(allocator, "Insertion into an empty array failed. See the dump for details.", MEMORIA_SOURCE);

			auto iter1 = Seek(array, 0);
			CheckBufferWritten(iter1, data, "Failed to read and compare buffer from array", MEMORIA_SOURCE);
		}
		else {
			if (step == 0)
			{
				//Insert at the start of the array
				auto iter = Seek(array, 0);

				BigInt len = array.GetSize();
				if (len > 100) len = 100;

				LeafPairsVector postfix(len);

				Read(iter, postfix);

				Skip(iter, -len);

				Insert(iter, data);

				Check(allocator, "Insertion at the start of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, -data.size());

				CheckBufferWritten(iter, data, "Failed to read and compare buffer from array", 				MEMORIA_SOURCE);
				CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 	MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//Insert at the end of the array
				auto iter = array.End();

				BigInt len = array.GetSize();
				if (len > 100) len = 100;

				LeafPairsVector prefix(len);
				Skip(iter, -len);

				Read(iter, prefix);

				Insert(iter, data);

				Check(allocator, "Insertion at the end of the array failed. See the dump for details.", MEMORIA_SOURCE);

				Skip(iter, -data.size() - len);

				CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", MEMORIA_SOURCE);
				CheckBufferWritten(iter, data, "Failed to read and compare buffer from array", 			MEMORIA_SOURCE);
			}
			else {
				//Insert in the middle of the array

				if (params->pos_ == -1) params->pos_ = GetRandomPosition(array);

				Int pos = params->pos_;

				auto iter = Seek(array, pos);

				if (params->page_step_ == -1) params->page_step_ = GetRandom(2);

				if (params->page_step_ == 0)
				{
					Skip(iter, -iter.key_idx());
					pos = iter.KeyNum();
				}

				BigInt prefix_len = pos;
				if (prefix_len > 100) prefix_len = 100;

				BigInt postfix_len = array.GetSize() - pos;
				if (postfix_len > 100) postfix_len = 100;

				LeafPairsVector prefix(prefix_len);
				LeafPairsVector postfix(postfix_len);

				Skip(iter, -prefix_len);

				Read(iter, prefix);
				Read(iter, postfix);

				Skip(iter, -postfix.size());

				Insert(iter, data);

				Check(allocator, "Insertion at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, - data.size() - prefix_len);

				CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 	MEMORIA_SOURCE);
				CheckBufferWritten(iter, data, 		"Failed to read and compare buffer from array", 		MEMORIA_SOURCE);
				CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", MEMORIA_SOURCE);
			}
		}
	}

	bool Remove(Allocator& allocator, SumSetCtr& array, SumSetBatchReplay* params)
	{
//		Int step = params->step_;
//
//		params->cnt_++;
//
//		if (array.Size() < 20000)
//		{
//			auto iter = array.Begin();
//			iter.Remove(array.Size());
//
//			Check(allocator, "Remove ByteArray", MEMORIA_SOURCE);
//			return array.Size() > 0;
//		}
//		else {
//			BigInt size = params->data_size_;
//
//			if (step == 0)
//			{
//				//Remove at the start of the array
//				auto iter = array.Seek(0);
//
//				BigInt len = array.Size() - size;
//				if (len > 100) len = 100;
//
//				ArrayData postfix(len);
//				iter.Skip(size);
//				iter.Read(postfix);
//				iter.Skip(-len - size);
//
//				iter.Remove(size);
//
//				Check(allocator, "Removing region at the start of the array failed. See the dump for details.", MEMORIA_SOURCE);
//
//				CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
//			}
//			else if (step == 1)
//			{
//				//Remove at the end of the array
//				auto iter = array.Seek(array.Size() - size);
//
//				BigInt len = iter.pos();
//				if (len > 100) len = 100;
//
//				ArrayData prefix(len);
//				iter.Skip(-len);
//				iter.Read(prefix);
//
//				iter.Remove(size);
//
//				Check(allocator, "Removing region at the end of the array failed. See the dump for details.", 	MEMORIA_SOURCE);
//
//				iter.Skip(-len);
//
//				CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", 		MEMORIA_SOURCE);
//			}
//			else {
//				//Remove at the middle of the array
//
//				if (params->pos_ == -1) params->pos_ = GetRandomPosition(array);
//
//				Int pos = params->pos_;
//
//				auto iter = array.Seek(pos);
//
//				if (params->page_step_ == -1) params->page_step_ = GetRandom(2);
//
//				if (params->page_step_ == 0)
//				{
//					iter.Skip(-iter.data_pos());
//					pos = iter.pos();
//				}
//
//				BigInt prefix_len = pos;
//				if (prefix_len > 100) prefix_len = 100;
//
//				BigInt postfix_len = array.Size() - (pos + size);
//				if (postfix_len > 100) postfix_len = 100;
//
//				ArrayData prefix(prefix_len);
//				ArrayData postfix(postfix_len);
//
//				iter.Skip(-prefix_len);
//
//				iter.Read(prefix);
//
//				iter.Skip(size);
//
//				iter.Read(postfix);
//
//				iter.Skip(-postfix.size() - size);
//
//				iter.Remove(size);
//
//				Check(allocator, "Removing region at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);
//
//				iter.Skip(-prefix_len);
//
//				CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 			MEMORIA_SOURCE);
//				CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
//			}
//
//			return array.Size() > 0;
//		}

		return false;
	}

};


}


#endif

