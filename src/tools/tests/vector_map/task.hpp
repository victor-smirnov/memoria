
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TEMPLATE_TASK_HPP_
#define MEMORIA_TESTS_TEMPLATE_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


#include "../shared/params.hpp"

namespace memoria {

using namespace memoria::vapi;

struct VectorMapReplay: public ReplayParams {
	Int 	data_;
	Int		data_size_;

	VectorMapReplay(): ReplayParams()
	{
		Add("data", data_);
		Add("dataSize", data_size_);
	}

};


struct VectorMapParams: public TestTaskParams {

	Int max_block_size_;

public:
	VectorMapParams(): TestTaskParams("VectorMap")
	{
		//Add("size", size_, 1024*100);
		Add("maxBlockSize", max_block_size_, 1024*40);
	}
};


class VectorMapTest: public SPTestTask {

	typedef StreamContainerTypesCollection::Factory<VectorMap>::Type 	VectorMapCtr;
	typedef VectorMapCtr::Iterator										VMIterator;

public:

	VectorMapTest(): SPTestTask(new VectorMapParams()) {}

	virtual ~VectorMapTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new VectorMapReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		DefaultLogHandlerImpl logHandler(out);

		VectorMapReplay* params = static_cast<VectorMapReplay*>(step_params);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		LoadAllocator(allocator, params);

		VectorMapCtr map(allocator, 1);

		ArrayData data = CreateBuffer(451, 0x55);

		auto iter = map.Create();

		iter.Insert(data);

		allocator.commit();

		StoreAllocator(allocator, "allocator1.dump");
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


		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		VectorMapCtr map(allocator, 1, true);

		try {

			params.step_ = 0;

			UByte value = 0;

			Int total_size = 0;

			for (Int c = 0; c < params.size_; c++, value++)
			{
				auto iter = map.Create();
				params.data_size_ = GetRandom(task_params->max_block_size_);
				total_size += params.data_size_;

				ArrayData data = CreateBuffer(params.data_size_, value);
				iter.Insert(data);

				Check(allocator, "Insertion failed.", 	MEMORIA_SOURCE);

				MEMORIA_TEST_ASSERT(map.array().Size(), != , total_size);

				allocator.commit();
			}

			StoreAllocator(allocator, "allocator.dump");
			Store(allocator, &params);
		}
		catch (...) {
			Store(allocator, &params);
			throw;
		}
	}

	void DoTestStep(ostream& out, Allocator& allocator, const VectorMapReplay* params)
	{



	}
};


}


#endif
