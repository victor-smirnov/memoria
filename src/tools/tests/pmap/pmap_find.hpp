
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_FIND_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_FIND_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;


struct PMapFindReplay: public TestReplayParams {
	PMapFindReplay(): TestReplayParams() {}
};


struct PMapFindParams: public TestTaskParams {
	PMapFindParams(): TestTaskParams("PMap.Find") {}
};


template <typename Key_, typename Value_, Int Blocks_ = 3>
struct PMapFindTypes {
	typedef Key_ 						Key;
	typedef Key_ 						IndexKey;
	typedef Value_						Value;

	static const Int Blocks 			= Blocks_;
	static const Int BranchingFactor	= 8;

	typedef Accumulators<Key, Blocks> 	Accumulator;
};





class PMapFindTest: public TestTask {

	typedef PMapFindTypes<Int, Int, 1> 		Types;

	typedef typename Types::Accumulator		Accumulator;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;

	static const Int Blocks					= Types::Blocks;

	typedef PackedTree<Types> 				Map;

public:

	PMapFindTest(): TestTask(new PMapFindParams()) {}

	virtual ~PMapFindTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new PMapFindReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{}

	void FillPMap(Map* map, Int size)
	{
		for (Int c = 0; c < size; c++)
		{
			map->key(0, c) = 2;
		}

		map->size() = size;

		map->Reindex(0);

		MEMORIA_TEST_THROW_IF(map->max_key(0), !=, size*2);
	}


	virtual void Run(ostream& out)
	{
		Int buffer_size 	= 1024*16;

		unique_ptr<Byte[]>	buffer_ptr(new Byte[buffer_size]);
		Byte* buffer 		= buffer_ptr.get();

		memset(buffer, buffer_size, 0);

		Map* map = T2T<Map*>(buffer);

		for (Int div = 1; div <= 16; div *= 2)
		{
			map->InitByBlock(buffer_size / div);

			for (Int size = 0; size < map->max_size(); size++)
			{
				map->key(0, size) = 2;
				map->size()++;
				map->Reindex(0, size, size + 1);

				for (Int c = 0; c < map->size(); c++)
				{
					Key src_key = (c + 1) * 2;

					MEMORIA_TEST_THROW_IF(c, !=, map->FindLE(0, src_key));
					MEMORIA_TEST_THROW_IF(c, !=, map->FindLE(0, src_key - 1));

					MEMORIA_TEST_THROW_IF(c, !=, map->FindLT(0, src_key - 1));

					if (c < map->size() - 1)
					{
						MEMORIA_TEST_THROW_IF(c + 1, !=, map->FindLT(0, src_key));
					}
					else {
						MEMORIA_TEST_THROW_IF(-1, !=, map->FindLT(0, src_key));
					}

					MEMORIA_TEST_THROW_IF(c,  !=, map->FindEQ(0, src_key));
					MEMORIA_TEST_THROW_IF(-1, !=, map->FindEQ(0, src_key - 1));
				}
			}
		}
	}
};


}


#endif
