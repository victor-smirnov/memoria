
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_MAP_APPEND_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_MAP_APPEND_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorMapAppendBenchmark: public SPBenchmarkTask {

	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<VectorMap>::Type 	MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;


	Allocator* 	allocator_;
	MapCtr* 	map_;

	BigInt 		memory_size;

public:

	VectorMapAppendBenchmark():
		SPBenchmarkTask("VectorMapAppend"), memory_size(128*1024*1024)
	{
		RootCtr::Init();
		MapCtr::Init();

		Add("memory_size", memory_size);
	}

	virtual ~VectorMapAppendBenchmark() throw() {}


	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ = new Allocator();

		map_ = new MapCtr(*allocator_, 1, true);
	}

	virtual void Release(ostream& out)
	{
		delete map_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();

		ArrayData data(size, malloc(size), true);

		BigInt total = 0;

		auto i = map_->End();

		while (total < memory_size)
		{
			map_->CreateNew(i);
			i.Insert(data);
			i++;

			total += data.size();
		}

		allocator_->rollback();

	}

	virtual String GetGraphName()
	{
		return "Memoria VectorMap Append";
	}
};


}


#endif
