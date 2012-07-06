
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_MAP_VECTOR_MAP_SEQUENTIAL_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_MAP_VECTOR_MAP_SEQUENTIAL_READ_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;




class VectorMapSequentialReadBenchmark: public SPBenchmarkTask {


	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<VectorMap>::Type 	Ctr;
	typedef typename Ctr::Iterator									Iterator;

	Allocator* 	allocator_;
	Ctr* 		ctr_;

	BigInt 		memory_size;

public:

	VectorMapSequentialReadBenchmark(StringRef name):
		SPBenchmarkTask(name), memory_size(128*1024*1024)
	{
		RootCtr::initMetadata();
		Ctr::initMetadata();

		Add("memory_size", memory_size);

		average = 10;
	}

	virtual ~VectorMapSequentialReadBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ = new Allocator();

		Int size = params.x();
		ArrayData data(size, malloc(size), true);

		BigInt total = 0;

		ctr_ = new Ctr(*allocator_, 1, true);

		while (total < memory_size)
		{
			auto i = ctr_->create();
			i.insert(data);
			total += data.size();
		}

		allocator_->commit();
	}

	virtual void release(ostream& out)
	{
		delete ctr_;
		delete allocator_;
	}

	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();
		ArrayData data(size, malloc(size), true);

		for (auto i = ctr_->Begin(); !i.isEnd(); i++)
		{
			i.read(data);
		}

		params.operations() = ctr_->count();
		params.memory() 	= ctr_->size() + ctr_->count() * 16; //sizeof(BigInt) * 2
	}
};


}


#endif
