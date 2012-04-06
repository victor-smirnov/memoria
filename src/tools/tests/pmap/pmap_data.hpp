
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_DATA_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_DATA_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_map2.hpp>

namespace memoria {




struct PMapReplay: public TestReplayParams {
	PMapReplay(): TestReplayParams(){}
};


struct PMapParams: public TestTaskParams {
	PMapParams(): TestTaskParams("PMap") {}
};


template <typename Key_, typename Value_, Int Blocks_ = 3>
struct PMapTypes {
	typedef Key_ 						Key;
	typedef Key_ 						IndexKey;
	typedef Value_						Value;

	static const Int Blocks 			= Blocks_;
	static const Int BranchingFactor	= 64 / sizeof (Key);

	typedef Accumulators<Key, Blocks> 	Accumulator;
};





class PMapDataTest: public TestTask {

	typedef PMapTypes<Int, Int, 3> 	Types;

	typedef typename Types::Accumulator		Accumulator;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;

	static const Int Blocks					= Types::Blocks;

	typedef PackedMap2<Types> 				Map;

public:

	PMapDataTest(): TestTask(new PMapParams()) {}

	virtual ~PMapDataTest() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new PMapReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		//PMapReplay* params = static_cast<PMapReplay*>(step_params);
	}

	void FillMap(Map* map)
	{
		for (Int c = 0; c < map->index_size() / 2; c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				map->index(d, c) = GetRandom(50000);
			}
		}

		for (Int c = 0; c < map->max_size() / 2; c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				map->key(d, c) = GetRandom(50000);
			}

			map->value(c) = GetRandom(50000);
		}

		map->size() = map->max_size() / 2;
	}

	void CopyMap(Map* src, Map* dst)
	{
		memmove(dst, src, src->GetObjectSize());
	}

	void CompareAfterInsert(Map* src, Map* dst, Int room_start, Int room_length)
	{
		MEMORIA_TEST_THROW_IF(src->size(), 			!=, dst->size() - room_length);
		MEMORIA_TEST_THROW_IF(src->max_size(), 		!=, dst->max_size());
		MEMORIA_TEST_THROW_IF(src->index_size(), 	!=, dst->index_size());

		for (Int c = 0; c < room_start; c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(src->key(d, c), !=, dst->key(d, c));
			}

			MEMORIA_TEST_THROW_IF(src->value(c), !=, dst->value(c));
		}


		for (Int c = room_start; c < src->size(); c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(src->key(d, c), !=, dst->key(d, c + room_length));
			}

			MEMORIA_TEST_THROW_IF(src->value(c), !=, dst->value(c + room_length));
		}


		for (Int c = 0; c < src->index_size(); c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(src->index(d, c), !=, dst->index(d, c));
			}
		}
	}

	void CompareEqual(Map* src, Map* dst, bool compare_index = true)
	{
		MEMORIA_TEST_THROW_IF(src->max_size(), 		!=, dst->max_size());
		MEMORIA_TEST_THROW_IF(src->index_size(), 	!=, dst->index_size());

		CompareContentEqual(src, dst);

		if (compare_index)
		{
			for (Int c = 0; c < src->index_size(); c++)
			{
				for (Int d = 0; d < Blocks; d++)
				{
					MEMORIA_TEST_THROW_IF(src->index(d, c), !=, dst->index(d, c));
				}
			}
		}
	}

	void CompareContentEqual(Map* src, Map* dst, bool cmp_max_size = true)
	{
		MEMORIA_TEST_THROW_IF(src->size(), !=, dst->size());

		for (Int c = 0; c < src->size(); c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF_1(src->key(d, c), !=, dst->key(d, c), c);
			}

			MEMORIA_TEST_THROW_IF(src->value(c), !=, dst->value(c));
		}
	}


	void CompareAfterRemove(Map* src, Map* dst, Int room_start, Int room_length)
	{
		MEMORIA_TEST_THROW_IF(src->size(), 			!=, dst->size() + room_length);
		MEMORIA_TEST_THROW_IF(src->max_size(), 		!=, dst->max_size());
		MEMORIA_TEST_THROW_IF(src->index_size(), 	!=, dst->index_size());

		for (Int c = 0; c < room_start; c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(src->key(d, c), !=, dst->key(d, c));
			}

			MEMORIA_TEST_THROW_IF(src->value(c), !=, dst->value(c));
		}


		for (Int c = room_start; c < src->size(); c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(src->key(d, c), !=, dst->key(d, c - room_length));
			}

			MEMORIA_TEST_THROW_IF(src->value(c), !=, dst->value(c - room_length));
		}


		for (Int c = 0; c < src->index_size(); c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(src->index(d, c), !=, dst->index(d, c));
			}
		}
	}



	void CompareAfterEnlarge(Map* src, Map* dst)
	{
		MEMORIA_TEST_THROW_IF(src->max_size(), ==, dst->max_size());
		CompareContentEqual(src, dst);
	}

	void CompareAfterShrink(Map* src, Map* dst)
	{
		MEMORIA_TEST_THROW_IF(src->max_size(), ==, dst->max_size());
		CompareContentEqual(src, dst);
	}


	virtual void Run(ostream& out)
	{
		Int buffer_size 	= 1024*16;

		Byte* buffer1 		= new Byte[buffer_size];
		Map* map1 			= T2T<Map*>(buffer1);

		map1->InitByBlock(buffer_size / 2);

		FillMap(map1);

		Byte* buffer2 		= new Byte[buffer_size];
		Map* map2 			= T2T<Map*>(buffer2);

		CopyMap(map1, map2);

		for (Int c = 0; c < map1->size(); c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(map1->key(d, c), !=, map2->key(d, c));
			}
		}


		for (Int c = 0; c < map1->index_size(); c++)
		{
			for (Int d = 0; d < Blocks; d++)
			{
				MEMORIA_TEST_THROW_IF(map1->index(d, c), !=, map2->index(d, c));
			}
		}

		Int room_start 	= map2->size() / 2;
		Int room_length = 10;

		map2->InsertSpace(room_start, room_length);

		CompareAfterInsert(map1, map2, room_start, room_length);

		map2->RemoveSpace(room_start + room_length, room_length);

		CompareEqual(map1, map2);

		map2->EnlargeBlock(buffer_size);

		CompareAfterEnlarge(map1, map2);

		map2->ShrinkBlock(buffer_size / 2);

		CompareEqual(map1, map2, false);
	}
};


}


#endif /* PMAP_DATA_HPP_ */
