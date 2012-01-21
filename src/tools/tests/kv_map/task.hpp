
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

class KVMapTestTask: public SPTestTask {
public:
	typedef KVPair<BigInt, BigInt> Pair;


private:
	typedef vector<Pair> PairVector;
	typedef StreamContainerTypesCollection::Factory<DefKVMap>::Type KVMapType;

	PairVector pairs;
	PairVector pairs_sorted;

public:

	KVMapTestTask(): SPTestTask(new KVMapTestTaskParams()) {}
	virtual ~KVMapTestTask() throw() {}

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
		typedef KVMapType::Iterator IteratorType;

		BigInt max = map->GetSize();

		for (Int c = 0; c < 100; c++)
		{
			IteratorType iter1 = map->Begin();
			IteratorType iter2 = iter1;

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
//		typedef KVMapType::Iterator IteratorType;
//
//		BigInt max = map->GetSize();
//
//		for (Int c = 0; c < 100; c++)
//		{
//			IteratorType iter1 = map->RBegin();
//			IteratorType iter2 = iter1;
//
//			BigInt rnd = max > 0 ? GetRandom(max) : 0;
//
//			auto iter3 = iter1;
//
//			if (rnd > 0) {
//				iter1.SkipKeyBw(rnd);
//			}
//
//			for (BigInt d = 0; d < rnd; d++)
//			{
//				iter2.PrevKey();
//			}
//
//			if (iter1 != iter2)
//			{
//				iter3.SkipKeyBw(rnd);
//			}
//
//			MEMORIA_TEST_ASSERT_EXPR(iter1 != iter2, iter1.key_idx(), iter2.key_idx());
//		}
	}

	virtual TestStepParams* CreateTestStep(StringRef name) const
	{
		return new KVMapTestStepParams(name);
	}

	virtual void Replay(ostream& out, TestStepParams* step_params)
	{
		KVMapTestStepParams* params = static_cast<KVMapTestStepParams*>(step_params);

		LoadVector(pairs, params->GetPairsDataFile());
		LoadVector(pairs_sorted, params->GetPairsSortedDataFile());

		Allocator allocator;
		LoadAllocator(allocator, params);

		DoTestStep(out, allocator, params);
	}

	virtual void Run(ostream& out)
	{
		Int SIZE = GetParameters<KVMapTestTaskParams>()->GetSize();

		for (Int c = 0; c < SIZE; c++)
		{
			pairs.push_back(Pair(GetBIRandom(), GetBIRandom()));
		}

		KVMapTestStepParams params;

		params.SetSize(SIZE);

		Allocator allocator;
		KVMapType map(allocator, 1, true);

		for (Int step = 0; step < 3; step++)
		{
			params.SetStep(step);

			for (Int c = 0; c < SIZE; c++)
			{
				PairVector pairs_sorted_tmp = pairs_sorted;

				try {
					params.SetVectorIdx(c);

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

	void StorePairs(const PairVector& pairs, const PairVector& pairs_sorted, KVMapTestStepParams& params)
	{
		String basic_name = GetTaskName()+ "." + params.GetName();

		String pairs_name = basic_name + ".pairs.txt";
		StoreVector(pairs, pairs_name);
		params.SetPairsDataFile(pairs_name);

		String pairs_sorted_name = basic_name + ".pairs_sorted.txt";
		StoreVector(pairs_sorted, pairs_sorted_name);
		params.SetPairsSortedDataFile(pairs_sorted_name);
	}

	void Check(Allocator& allocator)
	{
		Int level = allocator.GetLogger()->level();

		allocator.GetLogger()->level() = Logger::ERROR;

		memoria::StreamContainersChecker checker(allocator);
		if (checker.CheckAll())
		{
			throw TestException(MEMORIA_SOURCE, "Allocator check failed");
		}

		allocator.GetLogger()->level() = level;
	}

	void DoTestStep(ostream& out, Allocator& allocator, const KVMapTestStepParams* params)
	{
		unique_ptr<KVMapType> map(new KVMapType(allocator, 1));

		Int c = params->GetVectorIdx();

		if (params->GetStep() == 0)
		{
			map->Put(pairs[c].key_, pairs[c].value_);

			Check(allocator);

			AppendToSortedVector(pairs_sorted, pairs[c]);

			CheckIteratorFw(map.get(), pairs_sorted);
			CheckIteratorBw(map.get(), pairs_sorted);
			CheckMultistepForwardIterator(map.get());
			CheckMultistepBackwardIterator(map.get());

			allocator.commit();
		}
		else if (params->GetStep() == 1)
		{
			BigInt value = 0;
			map->Get1(pairs[c].key_, value);

			MEMORIA_TEST_ASSERT(pairs[c].value_, !=, value);
		}
		else {
			map->Remove(pairs[c].key_);

			Check(allocator);

			BigInt size = params->GetSize() - c - 1;

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


			if (c < params->GetSize() - 1)
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
