
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_PMAP_FIND_HPP_
#define MEMORIA_BENCHMARKS_PMAP_PMAP_FIND_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_map2.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;

struct PSetFindParams: public BenchmarkParams {
	Int iterations;
	PSetFindParams(Int BranchingFactor): BenchmarkParams("PSetFind."+ToString(BranchingFactor))
	{
		Add("iterations", iterations, 1*1024*1024);
	}
};


template <typename Key_, typename Value_, Int Blocks_, Int BranchingFactor_>
struct PMapFindTypes {
	typedef Key_ 						Key;
	typedef Key_ 						IndexKey;
	typedef Value_						Value;

	static const Int Blocks 			= Blocks_;
	static const Int BranchingFactor	= BranchingFactor_;

	typedef Accumulators<Key, Blocks> 	Accumulator;
};


template <typename List> class ListExecutor;


template <Int BranchingFactor>
class PSetBenchmark: public BenchmarkTask {

	typedef PMapFindTypes<Int, EmptyValue, 1, BranchingFactor> 	Types;

	typedef typename Types::Accumulator		Accumulator;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;

	static const Int Blocks					= Types::Blocks;

	typedef PackedMap2<Types> 				Map;

	Map* map_;

	Int* rd_array_;

	volatile Int result_;

	Byte* buffer_;
	bool dump_element_count_;

public:

	PSetBenchmark(Byte* buffer = NULL, bool dump_element_count = false):
		BenchmarkTask(new PSetFindParams(BranchingFactor)),
		buffer_(buffer),
		dump_element_count_(dump_element_count)
	{}

	virtual ~PSetBenchmark() throw() {}

	void FillPMap(Map* map, Int size)
	{
		for (Int c = 0; c < size; c++)
		{
			map->key(0, c) = 2;
		}

		map->size() = size;

		map->Reindex(0);
	}

	Int GetBufferSize() const
	{
		Int time = this->GetIteration();
		return 1024 * (1 << time);
	}

	virtual void Prepare(ostream& out)
	{
		PSetFindParams* params = GetParameters<PSetFindParams>();

		Int buffer_size 	= GetBufferSize();

		Byte* buffer;

		if (buffer_ == NULL)
		{
			buffer 		= T2T<Byte*>(malloc(buffer_size));
		}
		else {
			buffer 		= buffer_;
		}

		memset(buffer, buffer_size, 0);

		map_ = T2T<Map*>(buffer);

		map_->InitByBlock(buffer_size - sizeof(Map));

		FillPMap(map_, map_->max_size());

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			rd_array_[c] = (GetRandom(map_->size()) + 1)*2;
		}
	}

	virtual void Release(ostream& out)
	{
		if (buffer_ == NULL)
		{
			free(map_);
		}

		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		PSetFindParams* params = GetParameters<PSetFindParams>();

		for (Int c = 0; c < params->iterations; c++)
		{
			result_ = map_->FindEQ(0, rd_array_[c]);
		}

		if (dump_element_count_)
		{
			result.x() 			= map_->size();
		}
		else {
			result.x() 			= GetBufferSize()/1024;
		}

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "PackedSet<Int>, "+ToString(BranchingFactor)+" children";
	}
};


}


#endif
