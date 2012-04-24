
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_MISC_STL_MEMMOVE_HPP_
#define MEMORIA_BENCHMARKS_MISC_STL_MEMMOVE_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;

struct MemmoveParams: public BenchmarkParams {
	Int iterations;

	MemmoveParams(): BenchmarkParams("MemMove")
	{
		Add("iterations", iterations, 1*1024*1024);
	}
};


class MemmoveBenchmark: public BenchmarkTask {

	typedef BenchmarkTask Base;

	Byte* 	array_;
	Int		size_;
	Int* 	rd_array_;

public:

	MemmoveBenchmark():
		BenchmarkTask(new MemmoveParams())
	{
	}

	virtual ~MemmoveBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time)) * 4;
	}

	virtual void Prepare(ostream& out)
	{
		size_ = GetSetSize();

		array_ = T2T<Byte*>(malloc(size_));

		MemmoveParams* params = GetParameters<MemmoveParams>();

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			Int addr = GetRandom(size_ - 16);

			if ((addr & 4095) > 4080)
			{
				addr -= 16;
			}

			rd_array_[c] = addr;
		}
	}

	virtual void Release(ostream& out)
	{
		free (array_);
		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		StlVectorWriteParams* params = GetParameters<StlVectorWriteParams>();

		result.x() 			= size_/1024;

		BigInt total = 0;

		for (Int c = 0; c < params->iterations; c++)
		{
			Int addr = rd_array_[c];
			Int size = 4096 - (addr & 0xFFF);

			CopyBuffer(array_ + addr, array_ + addr + 8, size);

			total += size;
		}

		out<<"amount: "<<total/1024/1024<<endl;

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "memmove()";
	}
};


}


#endif
