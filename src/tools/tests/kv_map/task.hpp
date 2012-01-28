
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_KV_MAP_TASK_HPP_
#define MEMORIA_TESTS_KV_MAP_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>


#include "params.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

class KVMapTest: public SPTestTask {
public:
	typedef KVPair<BigInt, BigInt> Pair;


private:
	typedef vector<Pair> PairVector;
	typedef StreamContainerTypesCollection::Factory<DefKVMap>::Type KVMapType;

	PairVector pairs;
	PairVector pairs_sorted;

public:

	KVMapTest(): SPTestTask(new KVMapParams()) {}
	virtual ~KVMapTest() throw() {}

	void CheckIteratorFw(KVMapType* map, PairVector& pairs)
	{
		map->End().update();
		Int pairs_size = (Int)pairs.size();

		Int idx = 0;
		for (auto iter = map->Begin(); !iter.IsEnd(); )
		{
		    BigInt  key 	= iter.GetKey(0);
		    BigInt  value 	= iter.GetData();

		    MEMORIA_TEST_ASSERT1(pairs[idx].key_,   !=, key, idx);
		    MEMORIA_TEST_ASSERT1(pairs[idx].value_, !=, value, idx);

		    iter.Next();
		    idx++;
		}

		MEMORIA_TEST_ASSERT(idx, !=, pairs_size);

		idx = pairs_size - 1;
		for (auto iter = map->RBegin(); !iter.IsStart(); )
		{
			BigInt  key 	= iter.GetKey(0);
			BigInt  value 	= iter.GetData();

			MEMORIA_TEST_ASSERT1(pairs[idx].key_,   !=, key, idx);
			MEMORIA_TEST_ASSERT1(pairs[idx].value_, !=, value, idx);

			iter.Prev();

			idx--;
		}

		MEMORIA_TEST_ASSERT_EXPR(idx != -1, idx, pairs_size);
	}


	void CheckIteratorBw(KVMapType* map, PairVector& pairs)
	{
		Int pairs_size = (Int)pairs.size();
		Int idx = pairs_size - 1;

		for (auto iter = map->RBegin(); !iter.IsStart(); )
		{
			BigInt  key 	= iter.GetKey(0);
			BigInt  value 	= iter.GetData();

			MEMORIA_TEST_ASSERT(pairs[idx].key_,   !=, key);
			MEMORIA_TEST_ASSERT(pairs[idx].value_, !=, value);

		    iter.Prev();
		    idx--;
		}

		MEMORIA_TEST_ASSERT_EXPR(idx != -1, idx, pairs_size);
	}


	void CheckMultistepForwardIterator(KVMapType* map)
	{
		BigInt max = map->GetSize();

		for (Int c = 0; c < 100; c++)
		{
			auto iter1 = map->Begin();
			auto iter2 = iter1;

			BigInt rnd = max > 0 ? GetRandom(max) : 0;

			if (rnd > 0) iter1.SkipKeyFw(rnd);

			for (BigInt d = 0; d < rnd; d++)
			{
				iter2.NextKey();
			}

			MEMORIA_TEST_ASSERT_EXPR(iter1 != iter2, iter1.key_idx(), iter2.key_idx());
		}
	}

	//FIXME: SkipKeyBw is broken
	void CheckMultistepBackwardIterator(KVMapType* map)
	{
		BigInt max = map->GetSize();

		for (Int c = 0; c < 100; c++)
		{
			auto iter1 = map->RBegin();
			auto iter2 = iter1;

			BigInt rnd = max > 0 ? GetRandom(max) : 0;

			auto iter3 = iter1;

			if (rnd > 0) {
				iter1.SkipKeyBw(rnd);
			}

			for (BigInt d = 0; d < rnd; d++)
			{
				iter2.PrevKey();
			}

			MEMORIA_TEST_ASSERT_EXPR(iter1 != iter2, iter1.key_idx(), iter2.key_idx());
		}
	}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new KVMapReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		KVMapReplay* params = static_cast<KVMapReplay*>(step_params);

		LoadVector(pairs, params->pairs_data_file_);
		LoadVector(pairs_sorted, params->pairs_sorted_data_file_);

		Allocator allocator;
		LoadAllocator(allocator, params);

		DoTestStep(out, allocator, params);
	}

	virtual void Run(ostream& out)
	{
		DefaultLogHandlerImpl logHandler(out);

		KVMapParams* task_params = GetParameters<KVMapParams>();

		if (task_params->btree_random_airity_)
		{
			task_params->btree_airity_ = 8 + GetRandom(100);
			out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
		}

		Int SIZE 	= task_params->size_;

		pairs.clear();
		pairs_sorted.clear();

		for (Int c = 0; c < SIZE; c++)
		{
			pairs.push_back(Pair(GetUniqueBIRandom(pairs), GetBIRandom()));
		}

		KVMapReplay params;

		params.size_ = SIZE;
		params.btree_airity_ = task_params->btree_airity_;

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);

		KVMapType map(allocator, 1, true);

		map.SetMaxChildrenPerNode(params.btree_airity_);

		for (Int step = 0; step < 3; step++)
		{
			params.step_ = step;

			for (Int c = 0; c < SIZE; c++)
			{
				PairVector pairs_sorted_tmp = pairs_sorted;

				try {
					params.vector_idx_ = c;

					DoTestStep(out, allocator, &params);
				}
				catch (...)
				{
					StorePairs(pairs, pairs_sorted_tmp, params);
					Store(allocator, &params);
					throw;
				}
			}
		}
	}

	void StorePairs(const PairVector& pairs, const PairVector& pairs_sorted, KVMapReplay& params)
	{
		String basic_name =  "Data." + params.GetName();

		String pairs_name 		= basic_name + ".pairs.txt";
		params.pairs_data_file_ = pairs_name;

		StoreVector(pairs, pairs_name);

		String pairs_sorted_name 		= basic_name + ".pairs_sorted.txt";
		params.pairs_sorted_data_file_ 	= pairs_sorted_name;

		StoreVector(pairs_sorted, pairs_sorted_name);
	}



	void DoTestStep(ostream& out, Allocator& allocator, const KVMapReplay* params)
	{
		unique_ptr<KVMapType> map(new KVMapType(allocator, 1));

		map->SetMaxChildrenPerNode(params->btree_airity_);

		Int c = params->vector_idx_;

		if (params->step_ == 0)
		{
			map->Put(pairs[c].key_, pairs[c].value_);

			Check(allocator, MEMORIA_SOURCE);

			AppendToSortedVector(pairs_sorted, pairs[c]);

			CheckIteratorFw(map.get(), pairs_sorted);
			CheckIteratorBw(map.get(), pairs_sorted);
			CheckMultistepForwardIterator(map.get());
			CheckMultistepBackwardIterator(map.get());

			allocator.commit();
		}
		else if (params->step_ == 1)
		{
			BigInt value = 0;
			map->Get1(pairs[c].key_, value);

			MEMORIA_TEST_ASSERT(pairs[c].value_, !=, value);
		}
		else {
			map->Remove(pairs[c].key_);

			Check(allocator, MEMORIA_SOURCE);

			BigInt size = params->size_ - c - 1;

			MEMORIA_TEST_ASSERT(size, !=, map->GetSize());

			for (UInt x = 0; x < pairs_sorted.size(); x++)
			{
				if (pairs_sorted[x].key_ == pairs[c].key_)
				{
					pairs_sorted.erase(pairs_sorted.begin() + x);
				}
			}

			CheckIteratorFw(map.get(), pairs_sorted);
			CheckIteratorBw(map.get(), pairs_sorted);


			if (c < params->size_ - 1)
			{
				CheckMultistepForwardIterator(map.get());
				CheckMultistepBackwardIterator(map.get());
			}

			allocator.commit();
		}
	}
};


}


#endif
