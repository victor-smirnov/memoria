
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_INDEX_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_INDEX_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;


struct PMapIndexReplay: public TestReplayParams {
	PMapIndexReplay(): TestReplayParams() {}
};


struct PMapIndexParams: public TestTaskParams {
	PMapIndexParams(): TestTaskParams("PMap.Reindex") {}
};


template <typename Key_, typename Value_, Int Blocks_ = 3>
struct PMapReindexTypes {
	typedef Key_ 						Key;
	typedef Key_ 						IndexKey;
	typedef Value_						Value;

	static const Int Blocks 			= Blocks_;
	static const Int BranchingFactor	= 4;

	typedef Accumulators<Key, Blocks> 	Accumulator;
};





class PMapReindexTest: public TestTask {

	typedef PMapReindexTypes<Int, Int, 1> 	Types;

	typedef typename Types::Accumulator		Accumulator;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;

	static const Int Blocks					= Types::Blocks;

	typedef PackedSumTree<Types> 				Map;

public:

	PMapReindexTest(): TestTask(new PMapIndexParams()) {}

	virtual ~PMapReindexTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new PMapIndexReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{}

	BigInt FillPMap(Map* map, Int size)
	{
		BigInt sum = 0;

		for (Int c = 0; c < size; c++)
		{
			Int key = GetRandom(1000);
			map->key(0, c) = key;

			sum += key;
		}

		map->size() = size;

		map->Reindex(0);

		return sum;
	}


	virtual void Run(ostream& out)
	{
		Int buffer_size 	= 1024*64;

		unique_ptr<Byte[]>	buffer_ptr(new Byte[buffer_size]);
		Byte* buffer 		= buffer_ptr.get();

		memset(buffer, buffer_size, 0);

		Map* map = T2T<Map*>(buffer);

		for (int c = 1; c < 64; c++)
		{
			out<<"Reindex Pass: "<<c<<endl;

			map->InitByBlock(buffer_size / c);

			for (Int c = 1; c < map->max_size(); c++)
			{
				Int sum = FillPMap(map, c);
				MEMORIA_TEST_THROW_IF_1(map->max_key(0), !=, sum, c);
			}

			for (Int c = map->max_size() - 1; c > 0; c--)
			{
				Int sum = FillPMap(map, c);
				MEMORIA_TEST_THROW_IF_1(map->max_key(0), !=, sum, c);
			}
		}

		out<<endl<<endl;

		for (int c = 1; c < 64; c++)
		{
			out<<"Add Pass: "<<c<<endl;

			map->InitByBlock(buffer_size / c);

			Int sum = FillPMap(map, map->max_size());

			for (Int c = 0; c < map->max_size(); c++)
			{
				map->Add(0, c, 1);

				sum++;

				MEMORIA_TEST_THROW_IF_1(map->max_key(0), !=, sum, c);
			}
		}
	}
};


}


#endif
