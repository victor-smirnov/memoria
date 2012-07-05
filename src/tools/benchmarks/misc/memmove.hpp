
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_MISC_STL_MEMMOVE_HPP_
#define MEMORIA_BENCHMARKS_MISC_STL_MEMMOVE_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class MemmoveBenchmark: public BenchmarkTask {

	Byte* 	array_;
	Int* 	rd_array_;

public:

	MemmoveBenchmark(StringRef name):
		BenchmarkTask(name)
	{
		average	= 5;
	}

	virtual ~MemmoveBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();

		array_ = T2T<Byte*>(malloc(size));

		rd_array_ = new Int[params.operations()];
		for (Int c = 0; c < params.operations(); c++)
		{
			Int addr = getRandom(size - 16);

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

	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		BigInt 	total 	= 0;

		for (Int c = 0; c < params.operations(); c++)
		{
			Int addr = rd_array_[c];
			Int size = 4096 - (addr & 0xFFF) - 8;

			CopyBuffer(array_ + addr, array_ + addr + 8, size);

			total += size;
		}

		params.memory() = total;
	}
};


}


#endif
