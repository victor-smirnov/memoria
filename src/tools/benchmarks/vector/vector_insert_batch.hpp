
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_INSERT_BATCH_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_INSERT_BATCH_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;

class VectorInsertBatchBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Int max_size;

		Params(): BenchmarkParams("InsertBatch") {
			Add("max_size", max_size, 256*1024*1024);
		}
	};


	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<Vector>::Type 	MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;


	Allocator* 	allocator_;
	MapCtr* 	map_;



public:

	VectorInsertBatchBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~VectorInsertBatchBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return 1 << time;
	}

	virtual void Prepare(ostream& out)
	{
		allocator_ 	= new Allocator();
		map_ 		= new MapCtr(*allocator_, 1, true);

		allocator_->commit();
	}

	virtual void Release(ostream& out)
	{
		delete map_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		Params* params = GetParameters<Params>();

		Int size = GetSetSize();

		ArrayData data(size);

		Int max = params->max_size / size;

		Int total = 0;

		for (Int c = 0; c < max; c++)
		{
			auto i = map_->Seek(GetRandom(total));

			i.Insert(data);

			total += size;
		}

		result.x() 			= size;

		result.operations() = size;

		allocator_->rollback();
	}

	virtual String GetGraphName()
	{
		return String("Memoria Vector InsertBatch");
	}
};


}


#endif
