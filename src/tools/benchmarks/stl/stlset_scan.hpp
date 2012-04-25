
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_STLSET_SCAN_HPP_
#define MEMORIA_BENCHMARKS_PMAP_STLSET_SCAN_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_tree.hpp>

#include "custom_allocator.hpp"

#include <malloc.h>
#include <memory>
#include <set>



namespace memoria {

using namespace std;



class StlSetScanBenchmark: public BenchmarkTask {
public:
	enum SetType {MEMORY = 1, COUNT = 2};
private:


	struct Params: public BenchmarkParams {
		Params(Int kind): BenchmarkParams(String("StlFind")+(kind == 2? "Cnt" : "Mem")){}
	};

	typedef BigInt		 Key;
	typedef set<Key, less<Key>, CustomAllocator<Key> > Map;

	Map* 			map_;
	BigInt 			result_;
	SetType			set_type_;

public:

	StlSetScanBenchmark(SetType set_type): BenchmarkTask(new Params(set_type)), set_type_(set_type) {}

	virtual ~StlSetScanBenchmark() throw() {}

	Int GetBufferSize() const
	{
		Int time = this->GetIteration();
		return 1024 * (1 << time);
	}

	virtual void Prepare(ostream& out)
	{
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
	}

	virtual void Release(ostream& out)
	{
		delete map_;
	}



	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		Params* params = GetParameters<Params>();

		for (Int c = 0; c < params->iterations;)
		{
			for (auto i = map_->begin(); i!= map_->end() && c < params->iterations; i++, c++)
			{
				result_ += *i;
			}
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
		return "stl::set<int> (2 children) Scan";
	}
};


}


#endif
