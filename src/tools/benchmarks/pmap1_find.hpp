
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_PMAP1_FIND_HPP_
#define MEMORIA_BENCHMARKS_PMAP_PMAP1_FIND_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_map.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;


struct PMap1FindParams: public BenchmarkParams {
	Int iterations;
	PMap1FindParams(Int BranchingFactor): BenchmarkParams("PMap1.Find."+ToString(BranchingFactor))
	{
		Add("iterations", iterations, 1*1024*1024);
	}
};


template <typename Key_, typename Value_, Int Blocks_, Int BranchingFactor_, Int BlockSize_>
struct PMap1FindTypes {
	typedef Key_ 						Key;
	typedef Key_ 						IndexKey;
	typedef Value_						Value;

	static const Int Indexes 			= Blocks_;
	static const Int Children			= BranchingFactor_;
	static const Int BlockSize			= BlockSize_;
};


template <typename List> class ListExecutor;


template <Int BranchingFactor>
class PMap1Benchmark: public BenchmarkTask {

	typedef PackedIndexMapTypes<PMap1FindTypes<Int, EmptyValue, 1, BranchingFactor, 128*1024*1024> > 	Types;


	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;



	typedef PackedMap<Types> 				Map;

	Map* map_;

	volatile Int result_;

public:

	PMap1Benchmark(): BenchmarkTask(new PMap1FindParams(BranchingFactor))
	{
		Map::Init();
	}

	virtual ~PMap1Benchmark() throw() {}

	void FillPMap(Map* map, Int size)
	{
		for (Int c = 0; c < size; c++)
		{
			map->key(0, c) = 2;
		}

		map->size() = size;

		map->Reindex();
	}

//	Int GetBufferSize() const
//	{
//		Int time = this->GetIteration();
//		return 1024 * (1 << time);
//	}

	virtual void Prepare(ostream& out)
	{
		Int buffer_size 	= 128*1024*1204;

		Byte* buffer 		= T2T<Byte*>(malloc(buffer_size));

		memset(buffer, buffer_size, 0);

		map_ = T2T<Map*>(buffer);

		FillPMap(map_, map_->max_size());
	}

	virtual void Release(ostream& out)
	{
		delete map_;
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		PMapFindParams* params = GetParameters<PMapFindParams>();

		Int max = map_->size();

		for (Int c = 0; c < params->iterations; c++)
		{
			Int idx = GetRandom(max);

			result_ = map_->FindEQ(0, (idx + 1)*2);
		}

		result.x() 			= 128*1024;
		result.operations() = params->iterations;
	}
};


}


#endif
