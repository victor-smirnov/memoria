
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_STL_VECTOR_WRITE_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_STL_VECTOR_WRITE_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;




class StlVectorWriteBenchmark: public BenchmarkTask {

	typedef BenchmarkTask Base;

	struct Params: public BenchmarkParams {
		Params(): BenchmarkParams("StlVectorWrite"){}
	};


	typedef std::vector<BigInt> VectorCtr;

	VectorCtr* ctr_;

	Int* rd_array_;

public:

	StlVectorWriteBenchmark():
		BenchmarkTask(new Params())
	{
	}

	virtual ~StlVectorWriteBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time))/8;
	}

	virtual void Prepare(ostream& out)
	{
		Params* params = GetParameters<Params>();

		Int size = GetSetSize();

		ctr_ = new VectorCtr();

		for (Int c = 0; c < size; c++)
		{
			BigInt value = GetRandom(10000);
			ctr_->push_back(value);
		}

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			rd_array_[c] = GetRandom(size);
		}
	}

	virtual void Release(ostream& out)
	{
		delete ctr_;
		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		Params* params = GetParameters<Params>();

		result.x() 			= ctr_->size();

		for (Int c = 0; c < params->iterations; c++)
		{
			ctr_->insert(ctr_->begin() + rd_array_[c], rd_array_[c]);
		}

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "std::vector<BigInt>";
	}
};


}


#endif
