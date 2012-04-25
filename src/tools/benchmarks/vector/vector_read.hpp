
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_VECTOR_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_VECTOR_READ_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorReadBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Params(): BenchmarkParams("VectorRead") {}
	};


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

	volatile Int result_;

	Int* rd_array_;

public:

	VectorReadBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		VectorCtr::Init();
	}

	virtual ~VectorReadBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time))/8;
	}

	virtual void Prepare(ostream& out)
	{
		Params* params = GetParameters<Params>();

		allocator_ = new Allocator();

		Int size = GetSetSize();

		ctr_ = new VectorCtr(*allocator_);

		ctr_->SetElementSize(8);

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

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			rd_array_[c] = GetRandom(size);
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
		Params* params = GetParameters<Params>();

		for (Int c = 0; c < params->iterations; c++)
		{
			ctr_->Seek(rd_array_[c]);
		}

		result.x() 			= ctr_->Size();

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "Memoria Vector<BigInt>";
	}
};


}


#endif
