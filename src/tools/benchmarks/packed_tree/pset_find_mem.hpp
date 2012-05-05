
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_PMAP_FIND_MEM_HPP_
#define MEMORIA_BENCHMARKS_PMAP_PMAP_FIND_MEM_HPP_

#include "../benchmarks_inc.hpp"

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;





template <Int BranchingFactor_>
class PSetMemBenchmark: public BenchmarkTask {

	template <typename Key_, typename Value_, Int Blocks_>
	struct PMapFindTypes {
		typedef Key_ 						Key;
		typedef Key_ 						IndexKey;
		typedef Value_						Value;

		static const Int Blocks 			= Blocks_;
		static const Int BranchingFactor	= BranchingFactor_;

		typedef Accumulators<Key, Blocks> 	Accumulator;
	};


	typedef PMapFindTypes<BigInt, EmptyValue, 1> 				Types;

	typedef typename Types::Accumulator		Accumulator;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;

	static const Int Blocks					= Types::Blocks;

	typedef PackedSumTree<Types> 				Map;

	Map* map_;

	Int* rd_array_;

public:

	PSetMemBenchmark():
		BenchmarkTask("FindMem."+ToString(BranchingFactor_), "PackedSet<BigInt>, "+ToString(BranchingFactor_)+" children")
	{}

	virtual ~PSetMemBenchmark() throw() {}

	void FillPMap(Map* map, Int size)
	{
		for (Int c = 0; c < size; c++)
		{
			map->key(0, c) = 2;
		}

		map->size() = size;

		map->Reindex(0);
	}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		Int buffer_size 	= params.x();

		Byte* buffer 		= T2T<Byte*>(malloc(buffer_size));

		memset(buffer, buffer_size, 0);

		map_ = T2T<Map*>(buffer);

		map_->InitByBlock(buffer_size - sizeof(Map));

		FillPMap(map_, map_->max_size());

		rd_array_ = new Int[params.operations()];
		for (Int c = 0; c < params.operations(); c++)
		{
			rd_array_[c] = (GetRandom(map_->size()) + 1)*2;
		}
	}

	virtual void Release(ostream& out)
	{
		free(map_);
		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		for (Int c = 0; c < params.operations(); c++)
		{
			map_->FindEQ(0, rd_array_[c]);
		}
	}
};


}


#endif
