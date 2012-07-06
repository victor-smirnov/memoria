// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>


#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

class MapTest: public SPTestTask {

	struct MapReplay: public BTreeReplayParams {
		MapReplay(): BTreeReplayParams(){}
	};


public:
	typedef KVPair<BigInt, BigInt> Pair;

private:
	typedef vector<Pair> PairVector;
	typedef SmallCtrTypeFactory::Factory<Map1>::Type 				MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;
	typedef typename MapCtr::Accumulator							Accumulator;

	PairVector pairs;
	PairVector pairs_sorted;

public:

	MapTest() :
		SPTestTask("Map")
	{
		SmallCtrTypeFactory::Factory<Root>::Type::initMetadata();
		SmallCtrTypeFactory::Factory<Map1>::Type::initMetadata();
	}

	virtual ~MapTest() throw () {
	}

	void checkContainerData(MapCtr& map, PairVector& pairs)
	{
		Int pairs_size = (Int) pairs.size();

		Int idx = 0;
		for (auto iter = map.Begin(); !iter.IsEnd();)
		{
			BigInt key   = iter.getKey(0);
			BigInt value = iter.getValue();

			MEMORIA_TEST_THROW_IF_1(pairs[idx].key_,   !=, key, idx);
			MEMORIA_TEST_THROW_IF_1(pairs[idx].value_, !=, value, idx);

			iter.Next();
			idx++;
		}

		MEMORIA_TEST_THROW_IF	(idx, !=, pairs_size);

		idx = pairs_size - 1;
		for (auto iter = map.RBegin(); !iter.IsBegin(); )
		{
			BigInt  key 	= iter.getKey(0);
			BigInt  value 	= iter.getValue();

			MEMORIA_TEST_THROW_IF_1(pairs[idx].key_,   !=, key, idx);
			MEMORIA_TEST_THROW_IF_1(pairs[idx].value_, !=, value, idx);

			iter.Prev();

			idx--;
		}

		MEMORIA_TEST_THROW_IF_EXPR(idx != -1, idx, pairs_size);
	}



	virtual TestReplayParams* createTestStep(StringRef name) const
	{
		return new MapReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		MapReplay* params = static_cast<MapReplay*>(step_params);

		LoadVector(pairs, params->pairs_data_file_);
		LoadVector(pairs_sorted, params->pairs_sorted_data_file_);

		Allocator allocator;
		LoadAllocator(allocator, params);

		check(allocator, MEMORIA_SOURCE);

		DoTestStep(out, allocator, params);
	}

	virtual void Run(ostream& out)
	{
		DefaultLogHandlerImpl logHandler(out);

		MapTest* task_params = this;

		if (task_params->btree_random_branching_)
		{
			task_params->btree_branching_ = 8 + getRandom(100);
			out<<"BTree Branching: "<<task_params->btree_branching_<<endl;
		}

		Int SIZE = task_params->size_;

		pairs.clear();
		pairs_sorted.clear();

		for (Int c = 0; c < SIZE; c++)
		{
			pairs.push_back(Pair(getUniqueBIRandom(pairs, 1000000), getBIRandom(100000)));
		}

		MapReplay params;

		params.size_ = SIZE;

		Allocator allocator;
		allocator.getLogger()->setHandler(&logHandler);

		MapCtr map(allocator);

		params.ctr_name_ = map.name();

		map.setBranchingFactor(task_params->btree_branching_);

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

	void StorePairs(const PairVector& pairs, const PairVector& pairs_sorted, MapReplay& params)
	{
		String basic_name = "Data." + params.getName();

		String pairs_name = basic_name + ".pairs.txt";
		params.pairs_data_file_ = getResourcePath(pairs_name);

		StoreVector(pairs, params.pairs_data_file_);

		String pairs_sorted_name = basic_name + ".pairs_sorted.txt";
		params.pairs_sorted_data_file_ = getResourcePath(pairs_sorted_name);

		StoreVector(pairs_sorted, params.pairs_sorted_data_file_);
	}

	void DoTestStep(ostream& out, Allocator& allocator, const MapReplay* params)
	{
		MapCtr map(allocator, params->ctr_name_);

		Int c = params->vector_idx_;

		if (params->step_ == 0)
		{
			auto iter = map[pairs[c].key_];
			iter.setData(pairs[c].value_);

			checkIterator(out, iter, MEMORIA_SOURCE);

			check(allocator, MEMORIA_SOURCE);

			appendToSortedVector(pairs_sorted, pairs[c]);

			checkContainerData(map, pairs_sorted);

			allocator.commit();
		}
		else if (params->step_ == 1)
		{
			BigInt value = map[pairs[c].key_].getValue();

			MEMORIA_TEST_THROW_IF(pairs[c].value_, !=, value);
		}
		else {
			bool result = map.remove(pairs[c].key_);

			MEMORIA_TEST_THROW_IF(result, !=, true);

			check(allocator, MEMORIA_SOURCE);

			BigInt size = params->size_ - c - 1;

			MEMORIA_TEST_THROW_IF(size, !=, map.getSize());

			for (UInt x = 0; x < pairs_sorted.size(); x++)
			{
				if (pairs_sorted[x].key_ == pairs[c].key_)
				{
					pairs_sorted.erase(pairs_sorted.begin() + x);
				}
			}

			checkContainerData(map, pairs_sorted);

			allocator.commit();
		}
	}

	virtual void checkIterator(ostream& out, Iterator& iter, const char* source)
	{
		checkIteratorPrefix(out, iter, source);

		auto& path = iter.path();

		for (Int level = path.getSize() - 1; level > 0; level--)
		{
			bool found = false;

			for (Int idx = 0; idx < path[level]->children_count(); idx++)
			{
				ID id = iter.model().getINodeData(path[level].node(), idx);
				if (id == path[level - 1]->id())
				{
					if (path[level - 1].parent_idx() != idx)
					{
						iter.Dump(out);
						throw TestException(source, SBuf()<<"Invalid parent-child relationship for node:"<<path[level]->id()<<" child: "<<path[level - 1]->id()<<" idx="<<idx<<" parent_idx="<<path[level-1].parent_idx());
					}
					else {
						found = true;
						break;
					}
				}
			}

			if (!found)
			{
				iter.Dump(out);
				throw TestException(source, SBuf()<<"Child: "<<path[level - 1]->id()<<" is not fount is it's parent, parent_idx="<<path[level - 1].parent_idx());
			}
		}


	}

	virtual void checkIteratorPrefix(ostream& out, Iterator& iter, const char* source)
	{
		Accumulator prefix;

		iter.ComputePrefix(prefix);

		if (iter.prefix() != prefix.key(0))
		{
			iter.Dump(out);
			throw TestException(source, SBuf()<<"Invalid prefix value. Iterator: "<<iter.prefix()<<" Actual: "<<prefix);
		}
	}


};

}

#endif
