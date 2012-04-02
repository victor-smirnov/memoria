// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_KV_MAP_TASK_HPP_
#define MEMORIA_TESTS_KV_MAP_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/examples.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

struct CreateCtrParams: public ExampleTaskParams {

	CreateCtrParams(): ExampleTaskParams("CreateCtr") {}
};


class CreateCtrExample: public SPExampleTask {
public:
	typedef KVPair<BigInt, BigInt> Pair;

private:
	typedef vector<Pair> PairVector;
	typedef SmallCtrTypeFactory::Factory<Map1>::Type 				MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;
	typedef typename MapCtr::Accumulator							Accumulator;

	PairVector pairs;
	PairVector pairs_sorted;

public:

	CreateCtrExample() :
		SPExampleTask(new CreateCtrParams())
	{
		SmallCtrTypeFactory::Factory<Root>::Type::Init();
		SmallCtrTypeFactory::Factory<Map1>::Type::Init();
	}

	virtual ~CreateCtrExample() throw () {
	}


	virtual void Run(ostream& out)
	{
		DefaultLogHandlerImpl logHandler(out);

		CreateCtrParams* task_params = GetParameters<CreateCtrParams>();

		if (task_params->btree_random_airity_)
		{
			task_params->btree_branching_ = 8 + GetRandom(100);
			out<<"BTree Branching: "<<task_params->btree_branching_<<endl;
		}

		Int SIZE = task_params->size_;

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		MapCtr map(allocator);

		map.SetBranchingFactor(task_params->btree_branching_);

		for (Int c = 0; c < SIZE; c++)
		{
			map[c] = c;
		}

		allocator.commit();

		BigInt t0 = GetTimeInMillis();
		StoreAllocator(allocator, "/dev/null");
		BigInt t1 = GetTimeInMillis();

		out<<"Store Time: "<<FormatTime(t1 - t0)<<endl;
	}
};

}

#endif
