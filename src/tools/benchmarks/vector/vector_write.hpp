
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_VECTOR_WRITE_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_VECTOR_WRITE_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;

struct VectorWriteParams: public BenchmarkParams {
	Int iterations;

	VectorWriteParams(): BenchmarkParams("VectorWrite")
	{
		Add("iterations", iterations, 1*1024*1024);
	}
};


class VectorWriteBenchmark: public SPBenchmarkTask {

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





public:

	VectorWriteBenchmark():
		SPBenchmarkTask(new VectorWriteParams())
	{
		RootCtr::Init();
		VectorCtr::Init();
	}

	virtual ~VectorWriteBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time))/8;
	}

	virtual void Prepare(ostream& out)
	{
		allocator_ = new Allocator();

		Int size = GetSetSize();


		String resource_name = "allocator."+ToString(size)+".dump";

		if (IsResourceExists(resource_name))
		{
			LoadResource(*allocator_, resource_name);

			ctr_ = new VectorCtr(*allocator_, 1);
			ctr_->SetElementSize(8);
		}
		else {
			ctr_ = new VectorCtr(*allocator_, 1, true);

			Iterator i = ctr_->Seek(0);

			for (Int c = 0; c < size/128; c++)
			{
				BigInt array[128];
				for (Int d = 0; d < 128; d++)
				{
					array[d] = GetRandom(10000);
				}

				i.Insert(ArrayData(sizeof(array), array));
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
		VectorReadParams* params = GetParameters<VectorReadParams>();

		result.x() = ctr_->Size();

		for (Int c = 0; c < params->iterations; c++)
		{
			BigInt idx = GetRandom(ctr_->Size());
			Iterator i = ctr_->Seek(idx);

			if ((idx & 1) == 0)
			{
				i.Insert(idx);
			}
			else {
				i.Remove(1);
			}
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
