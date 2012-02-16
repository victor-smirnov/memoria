
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


	typedef typename SumSetCtr::NodeBaseG 					NodeBaseG;
	typedef typename SumSetCtr::NodeDispatcher 				NodeDispatcher;
	typedef typename SumSetCtr::ID 							ID;
	typedef typename SumSetCtr::LeafNodeKeyValuePair 		LeafNodeKeyValuePair;
	typedef typename SumSetCtr::NonLeafNodeKeyValuePair 	NonLeafNodeKeyValuePair;

	PairVector pairs;
	PairVector pairs_sorted;

	class SubtreeProvider: public SumSetCtr::ISubtreeProvider {

		typedef SumSetCtr::ISubtreeProvider::Enum Enum;

		BigInt total_;

	public:

		SubtreeProvider(BigInt total): total_(total) {}

		virtual NonLeafNodeKeyValuePair GetKVPair(SumSetCtr& ctr, Enum direction, BigInt begin, BigInt count, Int level)
		{
			NonLeafNodeKeyValuePair pair;

			return pair;
		}

		template <typename PairType, typename ParentPairType>
		struct SetNodeValuesFn
		{
			PairType* 		pairs_;
			Int 			from_;
			Int 			count_;

			ParentPairType	total_;

			SetNodeValuesFn(PairType* pairs, Int from, Int count): pairs_(pairs), from_(from), count_(count) {}

			template <typename Node>
			void operator()(Node* node)
			{
				for (Int c = from_; c < from_ + count_; c++)
				{
					for (Int d = 0; d < Indexes; d++)
					{
						node->map().keys(d, c) = pairs_[c].keys[d];
					}

					node->map().data(c) = pairs_[c].value;
				}

				node->map().Reindex();

				for (Int d = 0; d < Indexes; d++)
				{
					total_.keys[d] = node->map().max_keys(d);
				}

				total_.value = node->id();
			}
		};

		template <typename PairType>
		NonLeafNodeKeyValuePair SetNodeData(PairType* data, NodeBaseG& node, Int from, Int count)
		{
			SetNodeValuesFn<PairType, NonLeafNodeKeyValuePair> fn(data, from, count);
			NodeDispatcher::Dispatch(node, fn);
			return fn.total_;
		}

		NonLeafNodeKeyValuePair BuildLeftTree(SumSetCtr& ctr, Int level, BigInt& count)
		{
			NonLeafNodeKeyValuePair pair;

			Int max_keys = ctr.GetMaxKeyCountForNode(false, false, level);

			NonLeafNodeKeyValuePair children[1000];

			Int total = 0;
			for (Int c = 0; c < max_keys; c++, total++)
			{
				children[c] = BuildLeftTree(ctr, level - 1, count);
				if (count == 0)
				{
					break;
				}
			}

			NodeBaseG node = ctr.CreateNode(level, false, level == 0);

			SetNodeData(children, node, 0, total);

			//FIXME: leafs
			//FIXME: parent links

			return pair;
		}

		virtual LeafNodeKeyValuePair GetLeafKVPair(SumSetCtr& ctr, Enum direction, BigInt begin)
		{
			LeafNodeKeyValuePair pair;

			for (Int c = 0; c < Indexes; c++) pair.keys[c] = 1;

			return pair;
		}

		virtual BigInt 	GetTotalKeyCount() {
			return total_;
		}

	};


public:

	SumSetBatchTest(): SPTestTask(new SumSetParams("SumSetBatch")) 	{}
	virtual ~SumSetBatchTest() throw() 					{}

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
			//pairs.push_back(GetUniqueRandom(pairs));
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

		auto i1 = map.FindLE(1, 0, true);
		auto i2 = map.FindLE(10, 0, true);




		allocator.commit();

		StoreAllocator(allocator, "allocator2.dump");
	}
};


}


#endif
