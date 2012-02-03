
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TEMPLATE_TASK_HPP_
#define MEMORIA_TESTS_TEMPLATE_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


#include "../shared/params.hpp"


#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

struct VectorMapReplay: public ReplayParams {
	Int 	data_;
	Int		data_size_;

	String	pairs_data_file_;
	BigInt	key_;
	BigInt 	key_num_;

	VectorMapReplay(): ReplayParams()
	{
		Add("data", data_);
		Add("dataSize", data_size_);

		Add("pairsFile", pairs_data_file_);
		Add("key", key_);
		Add("keyNum", key_num_);
	}
};


struct VectorMapParams: public TestTaskParams {

	Int max_block_size_;

public:
	VectorMapParams(): TestTaskParams("VectorMap")
	{
		Add("maxBlockSize", max_block_size_, 1024*40);
	}
};


class VectorMapTest: public SPTestTask {

	typedef KVPair<BigInt, BigInt> 										Pair;
	typedef vector<Pair>												PairVector;
	typedef StreamContainerTypesCollection::Factory<VectorMap>::Type 	VectorMapCtr;
	typedef VectorMapCtr::Iterator										VMIterator;

	PairVector pairs_;

public:

	VectorMapTest(): SPTestTask(new VectorMapParams()) {}

	virtual ~VectorMapTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new VectorMapReplay();
	}

	void StorePairs(const PairVector& pairs, VectorMapReplay* params)
	{
		String basic_name = GetResourcePath("Data." + params->GetName());

		String pairs_name 		= basic_name + ".pairs.txt";
		params->pairs_data_file_ = pairs_name;

		StoreVector(pairs, pairs_name);
	}

	void CheckIteratorFw(VectorMapCtr& ctr)
	{
		MEMORIA_TEST_ASSERT(ctr.Count(), != , (BigInt)pairs_.size());

		auto iter = ctr.Begin();

		Int c = 0;
		for (Pair& pair: pairs_)
		{
			MEMORIA_TEST_ASSERT(iter.GetKey(), != , pair.key_);
			MEMORIA_TEST_ASSERT1(iter.size(),   != , pair.value_, c);
			MEMORIA_TEST_ASSERT(iter.pos(),    != , 0);

			UByte value = pair.key_ & 0xFF;

			ArrayData data = CreateBuffer(pair.value_, value);

			CheckBufferWritten(iter, data, "Buffer written does not match", MEMORIA_SOURCE);

			iter.Next();
			c++;
		}
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		pairs_.clear();

		DefaultLogHandlerImpl logHandler(out);

		VectorMapReplay* params = static_cast<VectorMapReplay*>(step_params);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		LoadAllocator(allocator, params);

		VectorMapCtr map(allocator, 1);

		if (params->step_ == 0)
		{

		}
		else if (params->step_ == 1)
		{
			LoadVector(pairs_, params->pairs_data_file_);

			BigInt key = params->key_;

			auto iter = map.Create(key);

			CheckCtr(map, "Insertion failed 1", MEMORIA_SOURCE);

			ArrayData data = CreateBuffer(params->data_size_, key & 0xFF);

			iter.Insert(data);

			AppendToSortedVector(pairs_, Pair(key, params->data_size_));

			CheckCtr(map, "Insertion failed 2", MEMORIA_SOURCE);

			MEMORIA_TEST_ASSERT(iter.size(), 	!= , data.size());
			MEMORIA_TEST_ASSERT(iter.GetKey(), 	!= , key);

			auto iter2 = map.Find(iter.GetKey());

			MEMORIA_TEST_ASSERT(iter2.exists(), != , true);
			MEMORIA_TEST_ASSERT(iter2.size(), 	!= , data.size());
			MEMORIA_TEST_ASSERT(iter2.GetKey(), != , iter.GetKey());

			CheckBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

			CheckIteratorFw(map);
		}
		else {
			LoadVector(pairs_, params->pairs_data_file_);

			MEMORIA_TEST_ASSERT(map.Count(), != , (BigInt)pairs_.size());

			Int idx		= params->key_num_;
			BigInt key  = params->key_;

			bool removed 		= map.Remove(key);

			MEMORIA_TEST_ASSERT(removed, != , true);

			CheckCtr(map, "Remove failed.", 	MEMORIA_SOURCE);

			pairs_.erase(pairs_.begin() + idx);

			CheckIteratorFw(map);
		}
	}

	virtual void Run(ostream& out)
	{
		VectorMapParams* task_params = GetParameters<VectorMapParams>();

		VectorMapReplay params;

		params.size_ = task_params->size_;
		if (task_params->btree_random_airity_)
		{
			task_params->btree_airity_ = 8 + GetRandom(100);
			out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
		}

		pairs_.clear();

//		TestOrderedCreation(out, &params);
//		TestRandomCreation(out, &params);
		TestRandomDeletion(out, &params);
	}

	void TestOrderedCreation(ostream& out, VectorMapReplay* params)
	{
		VectorMapParams* task_params = GetParameters<VectorMapParams>();
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		VectorMapCtr map(allocator, 1, true);

		try {

			params->step_ = 0;

			UByte value = 0;

			Int total_size = 0;

			for (Int c = 0; c < params->size_; c++, value++)
			{
				auto iter = map.Create();
				params->data_size_ = GetRandom(task_params->max_block_size_);

				ArrayData data = CreateBuffer(params->data_size_, c % 256);
				iter.Insert(data);

				MEMORIA_TEST_ASSERT(iter.size(), 	!= , data.size());
				MEMORIA_TEST_ASSERT(iter.GetKey(), 	!= , c + 1);

				total_size += iter.size();

				CheckCtr(map, "Insertion failed.", 	MEMORIA_SOURCE);

				MEMORIA_TEST_ASSERT(map.array().Size(), != , total_size);

				auto iter2 = map.Find(iter.GetKey());

				MEMORIA_TEST_ASSERT(iter2.exists(), != , true);
				MEMORIA_TEST_ASSERT(iter2.size(), 	!= , data.size());
				MEMORIA_TEST_ASSERT(iter2.GetKey(), != , iter.GetKey());

				CheckBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

				allocator.commit();
			}

			StoreAllocator(allocator, "allocator.dump");
			Store(allocator, params);
		}
		catch (...) {
			Store(allocator, params);
			throw;
		}
	}


	void TestRandomCreation(ostream& out, VectorMapReplay* params)
	{
		VectorMapParams* task_params = GetParameters<VectorMapParams>();
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		VectorMapCtr map(allocator, 1, true);

		PairVector pairs_tmp;

		pairs_.clear();

		try {
			params->step_ = 1;

			Int total_size = 0;

			for (Int c = 0; c < params->size_; c++)
			{
				BigInt key = GetUniqueRandom(pairs_);

				params->key_ 		= key;

				auto iter = map.Create(key);
				params->data_size_ = GetRandom(task_params->max_block_size_);

				ArrayData data = CreateBuffer(params->data_size_, key & 0xFF);
				iter.Insert(data);

				CheckCtr(map, "Insertion failed.", 	MEMORIA_SOURCE);

				params->key_num_ = AppendToSortedVector(pairs_, Pair(key, params->data_size_));

				MEMORIA_TEST_ASSERT(iter.size(), 	!= , data.size());
				MEMORIA_TEST_ASSERT(iter.GetKey(), 	!= , key);

				total_size += iter.size();

				MEMORIA_TEST_ASSERT(map.array().Size(), != , total_size);

				auto iter2 = map.Find(iter.GetKey());

				MEMORIA_TEST_ASSERT(iter2.exists(), != , true);
				MEMORIA_TEST_ASSERT(iter2.size(), 	!= , data.size());
				MEMORIA_TEST_ASSERT(iter2.GetKey(), != , iter.GetKey());

				CheckBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

				CheckIteratorFw(map);

				allocator.commit();

				AppendToSortedVector(pairs_tmp, Pair(key, params->data_size_));
			}

			MEMORIA_TEST_ASSERT(map.Count(), != , (BigInt)pairs_.size());
		}
		catch (...)
		{
			StorePairs(pairs_tmp, params);
			Store(allocator, params);

			throw;
		}
	}


	void TestRandomDeletion(ostream& out, VectorMapReplay* params)
	{
		VectorMapParams* task_params = GetParameters<VectorMapParams>();
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		VectorMapCtr map(allocator, 1, true);

		PairVector pairs_tmp;

		pairs_.clear();

		try {
			params->step_ = 2;

			for (Int c = 0; c < params->size_; c++)
			{
				BigInt key = GetUniqueRandom(pairs_);

				params->key_ 		= key;

				auto iter = map.Create(key);
				params->data_size_ = GetRandom(task_params->max_block_size_);

				ArrayData data = CreateBuffer(params->data_size_, key & 0xFF);
				iter.Insert(data);

				AppendToSortedVector(pairs_, Pair(key, params->data_size_));
			}

			allocator.commit();

			while (map.Count() > 0)
			{
				pairs_tmp = pairs_;

				Int idx = GetRandom(map.Count());

				params->key_num_ 	= idx;
				params->key_		= pairs_[idx].key_;

				bool removed = map.Remove(pairs_[idx].key_);

				MEMORIA_TEST_ASSERT(removed, != , true);

				CheckCtr(map, "Remove failed.", 	MEMORIA_SOURCE);

				pairs_.erase(pairs_.begin() + idx);

				CheckIteratorFw(map);
				allocator.commit();
			}

		}
		catch (...)
		{
			StorePairs(pairs_tmp, params);
			Store(allocator, params);
			throw;
		}
	}
};


}


#endif

