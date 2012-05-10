
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


#include "../shared/params.hpp"


#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;




class VectorMapTest: public SPTestTask {

	typedef KVPair<BigInt, BigInt> 										Pair;
	typedef vector<Pair>												PairVector;
	typedef SmallCtrTypeFactory::Factory<VectorMap>::Type 				VectorMapCtr;
	typedef VectorMapCtr::Iterator										VMIterator;

	struct TestReplay: public ReplayParams {
		Int 	data_;
		Int		data_size_;

		String	pairs_data_file_;
		BigInt	key_;
		BigInt 	key_num_;

		TestReplay(): ReplayParams()
		{
			Add("data", data_);
			Add("data_size", data_size_);

			Add("pairs_file", pairs_data_file_);
			Add("key", key_);
			Add("key_num", key_num_);
		}
	};


	PairVector pairs_;

	Int max_block_size_;

public:

	VectorMapTest():
		SPTestTask("VectorMap"),
		max_block_size_(1024*40)
	{
		VectorMapCtr::Init();

		Add("max_block_size", max_block_size_);
	}

	virtual ~VectorMapTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new TestReplay();
	}

	void StorePairs(const PairVector& pairs, TestReplay* params)
	{
		String basic_name = GetResourcePath("Data." + params->GetName());

		String pairs_name 		= basic_name + ".pairs.txt";
		params->pairs_data_file_ = pairs_name;

		StoreVector(pairs, pairs_name);
	}

	void CheckIteratorFw(VectorMapCtr& ctr)
	{
		MEMORIA_TEST_THROW_IF(ctr.Count(), != , (BigInt)pairs_.size());

		auto iter = ctr.Begin();

		Int c = 0;
		for (Pair& pair: pairs_)
		{
			MEMORIA_TEST_THROW_IF_1(iter.GetKey(), != , pair.key_, c);
			MEMORIA_TEST_THROW_IF_1(iter.size(),   != , pair.value_, c);
			MEMORIA_TEST_THROW_IF_1(iter.pos(),    != , 0, c);

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

		TestReplay* params = static_cast<TestReplay*>(step_params);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		LoadAllocator(allocator, params);

		VectorMapCtr map(allocator, params->ctr_name_);

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

			MEMORIA_TEST_THROW_IF(iter.size(), 	!= , data.size());
			MEMORIA_TEST_THROW_IF(iter.GetKey(), 	!= , key);

			auto iter2 = map.Find(iter.GetKey());

			MEMORIA_TEST_THROW_IF(iter2.exists(), != , true);
			MEMORIA_TEST_THROW_IF(iter2.size(), 	!= , data.size());
			MEMORIA_TEST_THROW_IF(iter2.GetKey(), != , iter.GetKey());

			CheckBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

			CheckIteratorFw(map);
		}
		else {
			LoadVector(pairs_, params->pairs_data_file_);

			MEMORIA_TEST_THROW_IF(map.Count(), != , (BigInt)pairs_.size());

			Int idx		= params->key_num_;
			BigInt key  = params->key_;

			bool removed 		= map.Remove(key);

			MEMORIA_TEST_THROW_IF(removed, != , true);

			CheckCtr(map, "Remove failed.", 	MEMORIA_SOURCE);

			pairs_.erase(pairs_.begin() + idx);

			CheckIteratorFw(map);
		}
	}

	virtual void Run(ostream& out)
	{
		VectorMapTest* task_params = this;

		TestReplay params;

		params.size_ = task_params->size_;
		if (task_params->btree_random_branching_)
		{
			task_params->btree_branching_ = 8 + GetRandom(100);
			out<<"BTree Branching: "<<task_params->btree_branching_<<endl;
		}

		pairs_.clear();

		TestOrderedCreation(out, &params);
		TestRandomCreation(out, &params);
		TestRandomDeletion(out, &params);
	}

	void TestOrderedCreation(ostream& out, TestReplay* params)
	{
		out<<"OrderedCreation Test"<<endl;

		VectorMapTest* task_params = this;
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		VectorMapCtr map(allocator);

		params->ctr_name_ = map.name();

		allocator.commit();

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

				MEMORIA_TEST_THROW_IF(iter.size(), 	 != , data.size());
				MEMORIA_TEST_THROW_IF(iter.GetKey(), != , c + 1);

				total_size += iter.size();

				CheckCtr(map, "Insertion failed.", 	MEMORIA_SOURCE);

				MEMORIA_TEST_THROW_IF(map.array().Size(), != , total_size);

				auto iter2 = map.Find(iter.GetKey());

				MEMORIA_TEST_THROW_IF(iter2.exists(), != , true);
				MEMORIA_TEST_THROW_IF(iter2.size(),   != , data.size());
				MEMORIA_TEST_THROW_IF(iter2.GetKey(), != , iter.GetKey());

				CheckBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

				allocator.commit();
			}
		}
		catch (...) {
			Store(allocator, params);
			throw;
		}
	}


	void TestRandomCreation(ostream& out, TestReplay* params)
	{
		out<<"RandomCreation test"<<endl;

		VectorMapTest* task_params = this;
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		VectorMapCtr map(allocator);

		params->ctr_name_ = map.name();

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

				MEMORIA_TEST_THROW_IF(iter.size(), 	!= , data.size());
				MEMORIA_TEST_THROW_IF(iter.GetKey(), 	!= , key);

				total_size += iter.size();

				MEMORIA_TEST_THROW_IF(map.array().Size(), != , total_size);

				auto iter2 = map.Find(iter.GetKey());

				MEMORIA_TEST_THROW_IF(iter2.exists(), != , true);
				MEMORIA_TEST_THROW_IF(iter2.size(), 	!= , data.size());
				MEMORIA_TEST_THROW_IF(iter2.GetKey(), != , iter.GetKey());

				CheckBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

				CheckIteratorFw(map);

				allocator.commit();

				AppendToSortedVector(pairs_tmp, Pair(key, params->data_size_));
			}

			MEMORIA_TEST_THROW_IF(map.Count(), != , (BigInt)pairs_.size());
		}
		catch (...)
		{
			StorePairs(pairs_tmp, params);
			Store(allocator, params);

			throw;
		}
	}


	void TestRandomDeletion(ostream& out, TestReplay* params)
	{
		out<<"RandomDeletion Test"<<endl;

		VectorMapTest* task_params = this;
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		VectorMapCtr map(allocator);

		params->ctr_name_ = map.name();

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

				MEMORIA_TEST_THROW_IF(removed, != , true);

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
