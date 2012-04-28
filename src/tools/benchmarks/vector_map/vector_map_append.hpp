
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_MAP_APPEND_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_MAP_APPEND_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorMapAppendBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Params(): BenchmarkParams("Append") {}
	};


	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<VectorMap>::Type 	MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;


	Allocator* allocator_;
	MapCtr* map_;



public:

	VectorMapAppendBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~VectorMapAppendBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * ((1) << time));
	}

	virtual void Prepare(ostream& out)
	{
		allocator_ = new Allocator();

		map_ = new MapCtr(*allocator_, 1, true);
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

		Byte buffer[256];

		auto i = map_->End();

		for (Int c = 0; c < size; c++)
		{
			map_->CreateNew(i);
			i.Insert(ArrayData(256, buffer));
			i++;
		}

		result.x() 			= map_->set().GetSize();

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return String("Memoria VectorMap Append");
	}
};


}


#endif
