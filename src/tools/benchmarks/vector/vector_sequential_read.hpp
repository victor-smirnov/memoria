
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_VECTOR_SEQUENTIAL_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_VECTOR_SEQUENTIAL_READ_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorSequentialReadBenchmark: public SPBenchmarkTask {


	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<Vector>::Type 	VectorCtr;
	typedef typename VectorCtr::Iterator							Iterator;
	typedef typename VectorCtr::ID									ID;
	typedef typename VectorCtr::Accumulator							Accumulator;


	typedef typename VectorCtr::Key									Key;



	Allocator* 	allocator_;
	VectorCtr* 	ctr_;

	BigInt		memory_size;

public:

	VectorSequentialReadBenchmark(StringRef graph_name = "Memoria Vector Sequential Read"):
		SPBenchmarkTask("VectorSequentialRead", graph_name), memory_size(128*1024*1024)
	{
		RootCtr::Init();
		VectorCtr::Init();

		Add("memory_size", memory_size);

		average = 5;
	}

	virtual ~VectorSequentialReadBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ = new Allocator();

		ctr_ = new VectorCtr(*allocator_);

		Iterator i = ctr_->Seek(0);

		ArrayData data(1024*1024, malloc(1024*1024), true);

		for (Int c = 0; c < memory_size / data.size(); c++)
		{
			i.Insert(data);
		}
	}

	virtual void Release(ostream& out)
	{
		delete ctr_;
		delete allocator_;
	}

	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		Int 	size 	= params.x();

		ArrayData data(size, malloc(size), true);

		for (Iterator i = ctr_->Seek(0); !i.IsEof();)
		{
			i.Read(data);
		}

		params.operations() = memory_size / size;
	}
};


}


#endif
