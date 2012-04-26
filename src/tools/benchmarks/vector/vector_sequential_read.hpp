
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_VECTOR_SEQUENTIAL_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_VECTOR_SEQUENTIAL_READ_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorSequentialReadBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Params(): BenchmarkParams("VectorSequentialRead") {}
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

public:

	VectorSequentialReadBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		VectorCtr::Init();
	}

	virtual ~VectorSequentialReadBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time));
	}

	virtual void Prepare(ostream& out)
	{
		allocator_ = new Allocator();

		Int size = GetSetSize();

		ctr_ = new VectorCtr(*allocator_);

		Iterator i = ctr_->Seek(0);

		Byte array[1024];

		for (Int c = 0; c < size/(Int)sizeof(array); c++)
		{
			for (Int d = 0; d < (Int)sizeof(array); d++)
			{
				array[d] = GetRandom(256);
			}

			i.Insert(ArrayData(sizeof(array), array));
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

		Byte array[4096];
		BigInt total = 0;

		for (Int c = 0; c < params->iterations;)
		{
			int a = 0; a++;
			for (Iterator i = ctr_->Seek(0); !i.IsEof() && c < params->iterations; c++)
			{
				ArrayData data(GetRandom(256), array);
				total += i.Read(data);
			}
		}

		cout<<"Total: "<<total/1024<<endl;

		result.x() 			= ctr_->Size();

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "Memoria Vector<BigInt> Sequential Read";
	}
};


}


#endif
