
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


	CreateCtrReplay(): ReplayParams()
	{

	}
};


struct CreateCtrParams: public TestTaskParams {

	Int map_size_;
	Int vector_map_size_;
	Int block_size_;

public:
	CreateCtrParams(): TestTaskParams("CreateCtr")
	{
		Add("MapSize", map_size_, 1024*1024*10);
		Add("VectorMapSize", vector_map_size_, 1024*1024*100);
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
		if (task_params->btree_random_airity_)
		{
			task_params->btree_airity_ = 8 + GetRandom(100);
			out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
		}

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		MapCtr map(allocator, 1, true);

		BigInt t00 = GetTimeInMillis();

		for (Int c = 0; c < task_params->map_size_; c++)
		{
			Int key = GetRandom();
			map[key].SetData(GetRandom());
		}

		VectorMapCtr vector_map(allocator, 2, true);

		free(NULL);
		FILE* fd = fopen("/dev/null", "wb");
		fclose(fd);

		for (Int c = 0; c < task_params->vector_map_size_; c++)
		{
			Int key = GetRandom();
			auto iter = vector_map.Create(key);

			ArrayData data = CreateBuffer(GetRandom(task_params->block_size_), GetRandom(256));
			iter.Insert(data);
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

		MapCtr new_map(new_alloc, 1);

		MEMORIA_TEST_THROW_IF(map.GetSize(), !=, new_map.GetSize());

		auto new_iter = new_map.Begin();

		for (auto iter = map.Begin(); !iter.IsEnd(); iter.NextKey(), new_iter.NextKey())
		{
			MEMORIA_TEST_THROW_IF(iter.GetKey(0), !=, new_iter.GetKey(0));
			MEMORIA_TEST_THROW_IF(iter.GetValue(), !=, new_iter.GetValue());
		}

		BigInt t22 = GetTimeInMillis();

		VectorMapCtr new_vector_map(new_alloc, 2);

		auto new_vm_iter = new_vector_map.Begin();

		for (auto iter = vector_map.Begin(); iter.IsNotEnd(); iter.Next(), new_vm_iter.Next())
		{
			MEMORIA_TEST_THROW_IF(iter.size(), !=, new_vm_iter.size());

			ArrayData data = CreateBuffer(iter.size(), 0);
			iter.Read(data);

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

