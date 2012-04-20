
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CREATE_CTR_TASK_HPP_
#define MEMORIA_TESTS_CREATE_CTR_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


#include "../shared/params.hpp"


#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

struct CreateCtrReplay: public ReplayParams {

	BigInt map_name_;
	BigInt vector_map_name_;

	CreateCtrReplay(): ReplayParams()
	{
		Add("map_name", map_name_);
		Add("vector_map_name", vector_map_name_);
	}
};


struct CreateCtrParams: public TestTaskParams {

	Int map_size_;
	Int vector_map_size_;
	Int block_size_;

public:
	CreateCtrParams(): TestTaskParams("CreateCtr")
	{
		Add("MapSize", map_size_, 1024*256);
		Add("VectorMapSize", vector_map_size_, 200);
		Add("BlockSize", block_size_, 1024);
	}
};


class CreateCtrTest: public SPTestTask {

	typedef KVPair<BigInt, BigInt> 										Pair;
	typedef vector<Pair>												PairVector;
	typedef SmallCtrTypeFactory::Factory<VectorMap>::Type 				VectorMapCtr;
	typedef SmallCtrTypeFactory::Factory<Map1>::Type 					MapCtr;
	typedef VectorMapCtr::Iterator										VMIterator;

	PairVector pairs_;

public:

	CreateCtrTest(): SPTestTask(new CreateCtrParams())
	{
		SmallCtrTypeFactory::Factory<Root>::Type::Init();
		VectorMapCtr::Init();
		MapCtr::Init();
	}

	virtual ~CreateCtrTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new CreateCtrReplay();
	}


	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
	}

	virtual void Run(ostream& out)
	{
		DefaultLogHandlerImpl logHandler(out);

		CreateCtrParams* task_params = GetParameters<CreateCtrParams>();

		CreateCtrReplay params;

		params.size_ = task_params->size_;
		if (task_params->btree_random_branching_)
		{
			task_params->btree_branching_ = 8 + GetRandom(100);
			out<<"BTree Branching: "<<task_params->btree_branching_<<endl;
		}

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		MapCtr map(allocator);

		map.SetBranchingFactor(100);

		params.map_name_ = map.name();

		BigInt t00 = GetTimeInMillis();

		for (Int c = 0; c < task_params->map_size_; c++)
		{
			map[GetRandom()] = GetRandom();
		}

		VectorMapCtr vector_map(allocator);

		vector_map.SetBranchingFactor(100);

		params.vector_map_name_ = vector_map.name();

		for (Int c = 0; c < task_params->vector_map_size_; c++)
		{
			vector_map[GetRandom()] = CreateBuffer(GetRandom(task_params->block_size_), GetRandom(256));
		}

		allocator.commit();

		BigInt t0 = GetTimeInMillis();

		StoreAllocator(allocator, "alloc1.dump");

		BigInt t1 = GetTimeInMillis();

		Allocator new_alloc;

		LoadAllocator(new_alloc, "alloc1.dump");

		BigInt t2 = GetTimeInMillis();

		out<<"Store Time: "<<FormatTime(t1 - t0)<<endl;
		out<<"Load Time:  "<<FormatTime(t2 - t1)<<endl;

		MapCtr new_map(new_alloc, map.name());

		MEMORIA_TEST_THROW_IF(map.GetBranchingFactor(), !=, new_map.GetBranchingFactor());

		MEMORIA_TEST_THROW_IF(map.GetSize(), !=, new_map.GetSize());

		auto new_iter = new_map.Begin();

		for (auto iter = map.Begin(); !iter.IsEnd(); iter.NextKey(), new_iter.NextKey())
		{
			MEMORIA_TEST_THROW_IF(iter.GetKey(0), !=, new_iter.GetKey(0));
			MEMORIA_TEST_THROW_IF(iter.GetValue(), !=, new_iter.GetValue());
		}

		BigInt t22 = GetTimeInMillis();

		VectorMapCtr new_vector_map(new_alloc, vector_map.name());

		MEMORIA_TEST_THROW_IF(vector_map.GetBranchingFactor(), !=, new_vector_map.GetBranchingFactor());

		auto new_vm_iter = new_vector_map.Begin();

		for (auto iter = vector_map.Begin(); iter.IsNotEnd(); iter.Next(), new_vm_iter.Next())
		{
			MEMORIA_TEST_THROW_IF(iter.size(), !=, new_vm_iter.size());

			ArrayData data = iter.Read();

			CheckBufferWritten(new_vm_iter, data, "Array data check failed", MEMORIA_SOURCE);
		}

		BigInt t33 = GetTimeInMillis();

		out<<"Create Time: "<<FormatTime(t0 - t00)<<endl;
		out<<"Check Time:  "<<FormatTime(t22 - t2)<<endl;
		out<<"Check Time:  "<<FormatTime(t33 - t22)<<endl;
	}


};


}


#endif

