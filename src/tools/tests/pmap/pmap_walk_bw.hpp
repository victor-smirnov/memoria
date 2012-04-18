
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_WALKBW_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_WALKBW_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;


struct PMapWalkBwReplay: public TestReplayParams {

	Int start;
	Int end;

	Int block_size;
	Int size;

	PMapWalkBwReplay(): TestReplayParams()
	{
		Add("start", 		start);
		Add("end", 			end);
		Add("block_size",	block_size);
		Add("size", 		size);
	}
};


struct PMapWalkBwParams: public TestTaskParams {

	Int block_size;
	Int max_size;

	PMapWalkBwParams(Int BranchingFactor): TestTaskParams("PMap.WalkBw."+ToString(BranchingFactor))
	{
		Add("block_size", block_size, 16384);
		Add("max_size",   max_size, 0);
	}
};


template <typename Key_, typename Value_, Int BF>
struct PMapWalkBwTypes {
	typedef Key_ 						Key;
	typedef Key_ 						IndexKey;
	typedef Value_						Value;

	static const Int Blocks 			= 1;
	static const Int BranchingFactor	= BF;

	typedef Accumulators<Key, Blocks> 	Accumulator;
};

template <Int BranchingFactor>
class PMapWalkBwTest: public TestTask {

	typedef PMapWalkBwTypes<Int, EmptyValue, BranchingFactor> 	Types;

	typedef typename Types::Accumulator		Accumulator;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;

	static const Int Blocks					= Types::Blocks;

	typedef PackedTree<Types> 				Map;

public:

	PMapWalkBwTest(): TestTask(new PMapWalkBwParams(BranchingFactor)) {}

	virtual ~PMapWalkBwTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new PMapWalkBwReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		PMapWalkBwReplay* params = T2T<PMapWalkBwReplay*>(step_params);

		Int start 		= params->start;
		Int end 		= params->end;
		Int size		= params->size;

		Int buffer_size 	= params->block_size;

		unique_ptr<Byte[]>	buffer_ptr(new Byte[buffer_size]);
		Byte* buffer 		= buffer_ptr.get();


		Map* map 			= T2T<Map*>(buffer);

		map->InitByBlock(buffer_size - sizeof(Map));

		FillMap(map, size);

		BigInt sum = Sum(map, start, end);

		Accumulator acc;
		Int idx = map->FindSumPositionBw(0, start, sum, acc);

		MEMORIA_TEST_THROW_IF_1(idx, !=, end, start);
	}

	void FillMap(Map* map, Int size)
	{
		for (Int c = 0; c < size; c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				map->key(d, c) = GetRandom(50) + 1;
			}
		}

		map->size() = size;

		map->ReindexAll(0, size);
	}


	BigInt Sum(Map* map, Int start, Int end) const
	{
		BigInt sum = 0;
		for (Int c = start; c > end; c--)
		{
			sum += map->key(0, c);
		}
		return sum;
	}

	virtual void Run(ostream& out)
	{
		Int buffer_size 	= GetParameters<PMapSumParams>()->block_size;
		Int max_size 		= GetParameters<PMapSumParams>()->max_size;

		unique_ptr<Byte[]>	buffer_ptr(new Byte[buffer_size]);
		Byte* buffer 		= buffer_ptr.get();


		Map* map 			= T2T<Map*>(buffer);

		map->InitByBlock(buffer_size - sizeof(Map));

		Int size = max_size != 0 ? max_size : map->max_size();

		FillMap(map, size);

		PMapWalkBwReplay replay;

		replay.block_size 	= buffer_size;
		replay.size			= size;

		try {
			for (Int end = map->size() - 1; end >= -1; end--)
			{
				for (Int start = map->size() - 1; start > end; start--)
				{
					replay.start 	= start;
					replay.end		= end;

					BigInt sum = Sum(map, start, end);

					Accumulator acc;
					Int idx = map->FindSumPositionBw(0, start, sum, acc);

					MEMORIA_TEST_THROW_IF_1(idx, !=, end, start);
				}
			}
		}
		catch (...) {
			Store(&replay);
			throw;
		}

	}
};


}


#endif /* PMAP_DATA_HPP_ */
