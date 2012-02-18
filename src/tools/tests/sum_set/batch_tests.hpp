
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
	typedef StreamContainerTypesCollection::Factory<SumSet1>::Type SumSetCtr;

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

	PairVector pairs;
	PairVector pairs_sorted;

	class SubtreeProvider2: public SumSetCtr::DefaultSubtreeProviderBase
	{
		typedef SumSetCtr::DefaultSubtreeProviderBase Base;

		const LeafNodeKeyValuePair* pairs_;
	public:
		SubtreeProvider2(SumSetCtr& ctr, BigInt total, const LeafNodeKeyValuePair* pairs): Base(ctr, total), pairs_(pairs) {}

		virtual LeafNodeKeyValuePair GetLeafKVPair(Direction direction, BigInt begin)
		{
			if (direction == Direction::FORWARD)
			{
				return pairs_[begin];
			}
			else {
				return pairs_[Base::GetTotalKeyCount() - begin - 1];
			}
		}
	};

	class SubtreeProvider: public SumSetCtr::DefaultSubtreeProviderBase
	{
		typedef SumSetCtr::DefaultSubtreeProviderBase Base;

	public:
		SubtreeProvider(SumSetCtr& ctr, BigInt total): Base(ctr, total) {}

		virtual LeafNodeKeyValuePair GetLeafKVPair(Direction direction, BigInt begin)
		{
			LeafNodeKeyValuePair pair;

			for (Int c = 0; c < Indexes; c++) pair.keys[c] = 2;

			return pair;
		}
	};

public:

	SumSetBatchTest(): SPTestTask(new SumSetParams("SumSetBatch")) 	{}
	virtual ~SumSetBatchTest() throw() 								{}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new SumSetReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{

	}

	virtual void Run(ostream& out)
	{
		DefaultLogHandlerImpl logHandler(out);

		SumSetParams* task_params = GetParameters<SumSetParams>();

		if (task_params->btree_random_airity_)
		{
			task_params->btree_airity_ = 8 + GetRandom(100);
			out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
		}

		Int SIZE 	= task_params->size_;

		pairs.clear();
		pairs_sorted.clear();

		Int sum = 0;
		for (Int c = 0; c < SIZE; c++, sum +=c)
		{
			pairs.push_back(sum);
		}

		SumSetReplay params;

		params.size_ = SIZE;
		params.btree_airity_ = task_params->btree_airity_;

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);
		SumSetCtr map(allocator, 1, true);
		map.SetMaxChildrenPerNode(params.btree_airity_);


		for (Int c = 1; c <= 10; c++)
		{
			map.Put(c, 0);
			out<<(c)<<" "<<sum<<endl;
		}

		allocator.commit();

		StoreAllocator(allocator, "allocator1.dump");

		auto i1 = map.FindLE(0, 0, true);

		LeafNodeKeyValuePair pairs[2000];

		for (Int c = 0; c < 2000; c++)
		{
			pairs[c].keys[0] = 4;
		}

		SubtreeProvider2 provider(map, 1000, pairs);

		map.InsertSubtree(i1, provider);

		//

		allocator.commit();

		StoreAllocator(allocator, "allocator2.dump");

		Check(allocator, MEMORIA_SOURCE);
	}
};


}


#endif

