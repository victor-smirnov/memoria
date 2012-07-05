
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_INSERT_BATCH_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_INSERT_BATCH_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;

class VectorInsertBatchBenchmark: public SPBenchmarkTask {
public:

	Int max_size;


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

	VectorInsertBatchBenchmark(StringRef name):
		SPBenchmarkTask(name), max_size(128*1024*1024)
	{
		RootCtr::Init();
		MapCtr::Init();

		Add("max_size", max_size);
	}

	virtual ~VectorInsertBatchBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
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


	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();

		ArrayData data(size, malloc(size), true);

		Int max = this->max_size / size;

		Int total = 0;

		params.operations() = max;

		for (Int c = 0; c < max; c++)
		{
			auto i = map_->Seek(getRandom(total));

			i.Insert(data);

			total += size;
		}

		allocator_->rollback();
	}
};


}


#endif
