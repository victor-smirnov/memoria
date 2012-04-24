
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_MAP_VECTOR_MAP_RANDOM_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_MAP_VECTOR_MAP_RANDOM_READ_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;

struct VectorMapRandomReadParams: public BenchmarkParams {
	Int iterations;

	VectorMapRandomReadParams(): BenchmarkParams("RandomRead")
	{
		Add("iterations", iterations, 1*1024*1024);
	}
};


class VectorMapRandomReadBenchmark: public SPBenchmarkTask {

	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<VectorMap>::Type 	Ctr;
	typedef typename Ctr::Iterator									Iterator;

	static const Int MAX_DATA_SIZE									= 256;

	Allocator* allocator_;
	Ctr* ctr_;

	volatile Int result_;

	Int* rd_array_;

public:

	VectorMapRandomReadBenchmark():
		SPBenchmarkTask(new VectorMapRandomReadParams())
	{
		RootCtr::Init();
		Ctr::Init();
	}

	virtual ~VectorMapRandomReadBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time))/8;
	}

	virtual void Prepare(ostream& out)
	{
		VectorMapRandomReadParams* params = GetParameters<VectorMapRandomReadParams>();

		allocator_ = new Allocator();

		Int size = GetSetSize();

		String resource_name = "VectorMap."+ToString(size)+".dump";

		if (IsResourceExists(resource_name))
		{
			LoadResource(*allocator_, resource_name);

			ctr_ = new Ctr(*allocator_, 1);
		}
		else {
			ctr_ = new Ctr(*allocator_, 1, true);

			Byte array[MAX_DATA_SIZE];

			for (Int c = 0; c < size; c++)
			{
				for (Int d = 0; d < 128; d++)
				{
					array[d] = GetRandom(256);
				}

				Iterator i = ctr_->Create();

				i = ArrayData(GetRandom(sizeof(array)), array);
			}

			allocator_->commit();
			StoreResource(*allocator_, resource_name);
		}

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			rd_array_[c] = GetRandom(size - 1) + 1;
		}
	}

	virtual void Release(ostream& out)
	{
		delete ctr_;
		delete allocator_;
		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		VectorMapRandomReadParams* params = GetParameters<VectorMapRandomReadParams>();

		Byte array[MAX_DATA_SIZE];
		ArrayData data(sizeof(array), array);

		result.x() 			= ctr_->Count();

		for (Int c = 0; c < params->iterations; c++)
		{
			ctr_->Find(rd_array_[c]).Read(data);
		}

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "Memoria Vector<BigInt>";
	}
};


}


#endif
