
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_STL_VECTOR_WRITE_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_STL_VECTOR_WRITE_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;


class StlVectorWriteBenchmark: public BenchmarkTask {

	typedef BenchmarkTask Base;

	typedef std::vector<BigInt> VectorCtr;

	VectorCtr* ctr_;

	Int* rd_array_;

public:

	StlVectorWriteBenchmark(StringRef name):
		BenchmarkTask(name)
	{
	}

	virtual ~StlVectorWriteBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();

		ctr_ = new VectorCtr();

		for (Int c = 0; c < size; c++)
		{
			BigInt value = getRandom(10000);
			ctr_->push_back(value);
		}

		rd_array_ = new Int[params.operations()];
		for (Int c = 0; c < params.operations(); c++)
		{
			rd_array_[c] = getRandom(size);
		}
	}

	virtual void release(ostream& out)
	{
		delete ctr_;
		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		for (Int c = 0; c < params.operations(); c++)
		{
			ctr_->insert(ctr_->begin() + rd_array_[c], rd_array_[c]);
		}
	}
};


}


#endif
