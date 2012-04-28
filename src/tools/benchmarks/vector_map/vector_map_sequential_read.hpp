
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_MAP_VECTOR_MAP_SEQUENTIAL_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_MAP_VECTOR_MAP_SEQUENTIAL_READ_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;




class VectorMapSequentialReadBenchmark: public SPBenchmarkTask {


	struct Params: public BenchmarkParams {
		Int iterations;

		Params(): BenchmarkParams("SequentialRead")
		{
			Add("iterations", iterations, 1*1024*1024);
		}
	};


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

public:

	VectorMapSequentialReadBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		Ctr::Init();
	}

	virtual ~VectorMapSequentialReadBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time))/8;
	}

	virtual void Prepare(ostream& out)
	{
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
	}

	virtual void Release(ostream& out)
	{
		delete ctr_;
		delete allocator_;
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		Params* params = GetParameters<Params>();

		Byte array[MAX_DATA_SIZE];
		ArrayData data(sizeof(array), array);

		result.x() 			= ctr_->Count();

		Iterator iter = ctr_->begin();

		for (Int c = 0; c < params->iterations;)
		{
			for (auto i = iter; i != ctr_->end() && c < params->iterations; i++, c++)
			{
				i.Read(data);
			}
		}

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "Memoria VectorMap Sequential";
	}
};


}


#endif
