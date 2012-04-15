
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_STLSET_FIND_HPP_
#define MEMORIA_BENCHMARKS_PMAP_STLSET_FIND_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_map2.hpp>

#include <malloc.h>
#include <memory>
#include <set>

namespace memoria {

using namespace std;


struct StlSetFindParams: public BenchmarkParams {
	Int iterations;
	Int	node_size;
	StlSetFindParams(Int kind): BenchmarkParams(String("StlFind")+(kind == 2? "Cnt" : "Mem"))
	{
		Add("iterations", iterations, 1*1024*1024);
		Add("node_size",  node_size,  (Int)(sizeof(void*) * 3 + sizeof(Int)));
	}
};


class StlSetBenchmark: public BenchmarkTask {
public:
	enum SetType {MEMORY = 1, COUNT = 2};
private:
	typedef set<BigInt> Map;

	Map* 			map_;
	volatile Int 	result_;
	SetType			set_type_;
	Int* 			rd_array_;

public:

	StlSetBenchmark(SetType set_type): BenchmarkTask(new StlSetFindParams(set_type)), set_type_(set_type) {}

	virtual ~StlSetBenchmark() throw() {}

	void FillPMap(Map* map, Int size)
	{
		BigInt sum = 2;

		for (Int c = 0; c < size; c++)
		{
			map->insert(sum+=2);
		}
	}

	Int GetBufferSize() const
	{
		Int time = this->GetIteration();
		return 1024 * (1 << time);
	}

	virtual void Prepare(ostream& out)
	{
		StlSetFindParams* params = GetParameters<StlSetFindParams>();

		Int buffer_size = GetBufferSize();

		map_ = new Map();

		if (set_type_ == COUNT)
		{
			FillPMap(map_, buffer_size / sizeof(BigInt));
		}
		else {
			FillPMap(map_, buffer_size / (params->node_size * 4));
		}

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			rd_array_[c] = (GetRandom(map_->size()) + 1)*2;
		}
	}

	virtual void Release(ostream& out)
	{
		free(map_);
		delete[] rd_array_;
	}



	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		StlSetFindParams* params = GetParameters<StlSetFindParams>();

		for (Int c = 0; c < params->iterations; c++)
		{
			result_ = (map_->find(rd_array_[c]) != map_->end());
		}


		if (set_type_ == MEMORY)
		{
			result.x() 			= GetBufferSize()/1024;
		}
		else {
			result.x() 			= map_->size();
		}


		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "stl::set<int> (2 children)";
	}
};


}


#endif
