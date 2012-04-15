
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_STLUSET_FIND_HPP_
#define MEMORIA_BENCHMARKS_PMAP_STLUSET_FIND_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_map2.hpp>

#include "custom_allocator.hpp"

#include <malloc.h>
#include <memory>
#include <unordered_set>

namespace memoria {

using namespace std;

struct StlUSetFindParams: public BenchmarkParams {
	Int iterations;

	StlUSetFindParams(Int kind): BenchmarkParams(String("UStlFind")+(kind == 2? "Cnt" : "Mem"))
	{
		Add("iterations", iterations, 1*1024*1024);
	}
};


class StlUSetBenchmark: public BenchmarkTask {
public:
	enum SetType {MEMORY = 1, COUNT = 2};
private:

	typedef Int		 Key;
	typedef unordered_set<
				Key,
            	hash<Key>,
            	std::equal_to<Key>,
            	CustomAllocator<Key>
	> Map;

	Map* 			map_;
	volatile Int 	result_;
	SetType			set_type_;
	Int* 			rd_array_;

public:

	StlUSetBenchmark(SetType set_type): BenchmarkTask(new StlUSetFindParams(set_type)), set_type_(set_type) {}

	virtual ~StlUSetBenchmark() throw() {}

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
			BigInt sum = 2;

			for (Int c = 0; c < buffer_size / (Int)sizeof(Key); c++)
			{
				map_->insert(sum+=2);
			}
		}
		else
		{
			AllocatorBase<>::reset();

			BigInt sum = 2;
			while (AllocatorBase<>::count() < buffer_size)
			{
				map_->insert(sum+=2);
			}

			out<<"Buffer: "<<buffer_size<<" elements: "<<map_->size()<<endl;
		}

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			rd_array_[c] = (GetRandom(map_->size()) + 1)*2;
		}
	}

	virtual void Release(ostream& out)
	{
		delete map_;
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
		return "stl::unordered_set<int>";
	}
};


}


#endif
