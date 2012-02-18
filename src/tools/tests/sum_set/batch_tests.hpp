
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

	class SubtreeProvider: public SumSetCtr::ISubtreeProvider {

		typedef SumSetCtr::ISubtreeProvider::Enum Direction;

		BigInt total_;

	public:

		SubtreeProvider(BigInt total): total_(total) {}

		virtual NonLeafNodeKeyValuePair GetKVPair(SumSetCtr& ctr, Direction direction, BigInt begin, BigInt total, Int level)
		{
			BigInt local_count = 0;
			return BuildTree(ctr, direction, local_count, total, level - 1);
		}



		NonLeafNodeKeyValuePair BuildTree(SumSetCtr& ctr, Direction direction, BigInt& count, const BigInt total, Int level)
		{
			NonLeafNodeKeyValuePair pair;
			pair.key_count = 0;

			Int max_keys = ctr.GetMaxKeyCountForNode(false, level == 0, level);

			if (level > 0)
			{
				NonLeafNodeKeyValuePair children[1000];

				Int local = 0;
				for (Int c = 0; c < max_keys && count < total; c++, local++)
				{
					children[c] 	=  BuildTree(ctr, direction, count, total, level - 1);
					pair.key_count 	+= children[c].key_count;
				}

				if (direction == Direction::BACKWARD)
				{
					SwapVector(children, total);
				}

				NodeBaseG node = ctr.CreateNode(level, false, false);

				SetINodeData(children, node, local);
				ctr.UpdateParentLinksAndCounters(node);

				ctr.GetMaxKeys(node, pair.keys);
				pair.value = node->id();
			}
			else
			{
				LeafNodeKeyValuePair children[1000];

				Int local = 0;
				for (Int c = 0; c < max_keys && count < total; c++, local++, count++)
				{
					children[c] 	=  this->GetLeafKVPair(ctr, direction, count);
				}

				if (direction == Direction::BACKWARD)
				{
					SwapVector(children, total);
				}

				NodeBaseG node = ctr.CreateNode(level, false, true);

				SetLeafNodeData(children, node, local);
				ctr.GetMaxKeys(node, pair.keys);

				node->counters().key_count() = local;

				pair.value 		=  node->id();
				pair.key_count 	+= total;
			}

			return pair;
		}


		virtual LeafNodeKeyValuePair GetLeafKVPair(SumSetCtr& ctr, Direction direction, BigInt begin)
		{
			LeafNodeKeyValuePair pair;

			for (Int c = 0; c < Indexes; c++) pair.keys[c] = 1;

			return pair;
		}

		virtual BigInt 	GetTotalKeyCount() {
			return total_;
		}


		template <typename PairType, typename ParentPairType>
		struct SetNodeValuesFn
		{
			PairType* 		pairs_;
			Int 			count_;

			ParentPairType	total_;

			SetNodeValuesFn(PairType* pairs, Int count): pairs_(pairs), count_(count) {}

			template <typename Node>
			void operator()(Node* node)
			{
				for (Int c = 0; c < count_; c++)
				{
					for (Int d = 0; d < Indexes; d++)
					{
						node->map().key(d, c) = pairs_[c].keys[d];
					}

					node->map().data(c) = pairs_[c].value;
				}

				node->set_children_count(count_);

				node->map().Reindex();

				for (Int d = 0; d < Indexes; d++)
				{
					total_.keys[d] = node->map().max_key(d);
				}

				total_.value = node->id();
			}
		};

		template <typename PairType>
		NonLeafNodeKeyValuePair SetINodeData(PairType* data, NodeBaseG& node, Int count)
		{
			SetNodeValuesFn<PairType, NonLeafNodeKeyValuePair> fn(data, count);
			NonLeafDispatcher::Dispatch(node, fn);
			return fn.total_;
		}

		template <typename PairType>
		NonLeafNodeKeyValuePair SetLeafNodeData(PairType* data, NodeBaseG& node, Int count)
		{
			SetNodeValuesFn<PairType, NonLeafNodeKeyValuePair> fn(data, count);
			LeafDispatcher::Dispatch(node, fn);
			return fn.total_;
		}




		template <typename PairType>
		void SwapVector(PairType* data, Int total)
		{
			for (Int c = 0; c < total; c++)
			{
				PairType tmp 			= data[c];
				data[c] 				= data[total - c  - 1];
				data[total - c  - 1] 	= tmp;
			}
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

		auto i1 = map.FindLE(2, 0, true);
//		auto i2 = map.FindLE(10, 0, true);

		SubtreeProvider provider(1024);

		map.InsertSubtree(i1, provider);

		allocator.commit();

		StoreAllocator(allocator, "allocator2.dump");
	}
};


}


#endif
