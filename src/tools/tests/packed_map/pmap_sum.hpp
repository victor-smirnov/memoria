
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_SUM_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_SUM_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;



template <Int BranchingFactor_>
class PMapSumTest: public TestTask {

	template <typename Key_, typename Value_, Int BF>
	struct PMapSumTypes {
		typedef Key_ 						Key;
		typedef Key_ 						IndexKey;
		typedef Value_						Value;

		static const Int Blocks 			= 1;
		static const Int BranchingFactor	= BF;

		typedef Accumulators<Key, Blocks> 	Accumulator;
	};


	struct TestReplay: public TestReplayParams {

		Int start;
		Int end;
		Int block_size;
		Int max_size;

		TestReplay(): TestReplayParams()
		{
			Add("start", 		start);
			Add("end", 			end);
			Add("block_size",	block_size);
			Add("max_size", 	max_size);
		}
	};

	typedef PMapSumTypes<BigInt, EmptyValue, BranchingFactor_> 	Types;

	typedef typename Types::Accumulator		Accumulator;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;

	static const Int Blocks					= Types::Blocks;

	typedef PackedSumTree<Types> 				Map;

	Int block_size;
	Int max_size;

public:

	PMapSumTest():
		TestTask("Sum."+toString(BranchingFactor_)),
		block_size(16384),
		max_size(0)
	{
		Add("block_size", block_size);
		Add("max_size", max_size);
	}

	virtual ~PMapSumTest() throw() {}

	virtual TestReplayParams* createTestStep(StringRef name) const
	{
		return new TestReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		TestReplay* params = T2T<TestReplay*>(step_params);

		Int start 		= params->start;
		Int end 		= params->end;
		Int max_size	= params->max_size;

		Int buffer_size 	= params->block_size;

		unique_ptr<Byte[]>	buffer_ptr(new Byte[buffer_size]);
		Byte* buffer 		= buffer_ptr.get();


		Map* map 			= T2T<Map*>(buffer);

		map->initByBlock(buffer_size - sizeof(Map));

		FillMap(map, max_size != 0 ? max_size : map->maxSize());

		Accumulator acc;
		map->Sum(start, end, acc);

		BigInt sum = Sum(map, start, end);

		MEMORIA_TEST_THROW_IF_1(acc[0], !=, sum, toString(start) + "," + toString(end));
	}

	void FillMap(Map* map, Int size)
	{
		for (Int c = 0; c < size; c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				map->key(d, c) = getRandom(50);
			}
		}

		map->size() = size;

		map->reindexAll(0, size);
	}


	BigInt Sum(Map* map, Int start, Int end) const
	{
		BigInt sum = 0;
		for (Int c = start; c < end; c++)
		{
			sum += map->key(0, c);
		}
		return sum;
	}

	virtual void Run(ostream& out)
	{
		Int buffer_size 	= this->block_size;
		Int max_size 		= this->max_size;

		unique_ptr<Byte[]>	buffer_ptr(new Byte[buffer_size]);
		Byte* buffer 		= buffer_ptr.get();


		Map* map 			= T2T<Map*>(buffer);

		map->initByBlock(buffer_size - sizeof(Map));

		FillMap(map, max_size != 0 ? max_size : map->maxSize());

		TestReplay replay;
		replay.block_size = buffer_size;

		try {
			for (Int end = 0; end < map->size(); end++)
			{
				out<<end<<endl;

				for (Int start = 0; start < end; start++)
				{
					replay.start 	= start;
					replay.end		= end;

					Accumulator acc;
					map->Sum(start, end, acc);

					BigInt sum = Sum(map, start, end);

					MEMORIA_TEST_THROW_IF_1(acc[0], !=, sum, toString(start) + ","+toString(end));
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
