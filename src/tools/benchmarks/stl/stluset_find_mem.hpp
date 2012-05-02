
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_STLUSET_FIND_MEM_HPP_
#define MEMORIA_BENCHMARKS_PMAP_STLUSET_FIND_MEM_HPP_

#include "../benchmarks_inc.hpp"

#include "custom_allocator.hpp"

#include <malloc.h>
#include <memory>
#include <unordered_set>

namespace memoria {

using namespace std;




class StlUSetMemBenchmark: public BenchmarkTask {


	typedef BigInt							Key;
	typedef unordered_set<
				Key,
            	hash<Key>,
            	std::equal_to<Key>,
            	CustomAllocator<Key>
	> 										Map;

	Map* 			map_;
	Int 			result_;
	Int* 			rd_array_;

public:

	StlUSetMemBenchmark(): BenchmarkTask("StlUSetMem.Find") {}

	virtual ~StlUSetMemBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		map_ = new Map();

		AllocatorBase<>::reset();

		BigInt sum = 0;
		while (AllocatorBase<>::count() < params.x())
		{
			map_->insert(sum++);
		}

		rd_array_ = new Int[params.operations()];
		for (Int c = 0; c < params.operations(); c++)
		{
			rd_array_[c] = GetRandom(map_->size());
		}
	}

	virtual void Release(ostream& out)
	{
		delete map_;
		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		for (Int c = 0; c < params.operations(); c++)
		{
			result_ = (map_->find(rd_array_[c]) != map_->end());
		}
	}

	virtual String GetGraphName()
	{
		return "std::unordered_set<BigInt>";
	}
};


}


#endif
