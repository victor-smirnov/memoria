
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_VECTOR_WRITE_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_VECTOR_WRITE_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorRandominsertBenchmark: public SPBenchmarkTask {

	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<Vector>::Type 	VectorCtr;
	typedef typename VectorCtr::Iterator							Iterator;
	typedef typename VectorCtr::ID									ID;
	typedef typename VectorCtr::Accumulator							Accumulator;


	typedef typename VectorCtr::Key									Key;

	Allocator* allocator_;
	VectorCtr* ctr_;

	BigInt memory_size;

public:

	VectorRandominsertBenchmark(StringRef name):
		SPBenchmarkTask(name), memory_size(128*1024*1024)
	{
		RootCtr::initMetadata();
		VectorCtr::initMetadata();

		Add("memory_size", memory_size);
	}

	virtual ~VectorRandominsertBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ 	= new Allocator();
		ctr_ 		= new VectorCtr(*allocator_, 1, true);

		allocator_->commit();
	}

	virtual void Release(ostream& out)
	{
		delete ctr_;
		delete allocator_;
	}

	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		BigInt total = 0;

		ArrayData data(params.x(), malloc(params.x()), true);

		Int cnt = 0;

		params.operations() = 0;

		while (total < memory_size)
		{
			BigInt idx = getRandom(total);
			Iterator i = ctr_->seek(idx);
			i.insert(data);
			total += data.size();

			cnt++;

			params.operations()++;
		}

		params.memory() = total;

		allocator_->rollback();
	}
};


}


#endif
