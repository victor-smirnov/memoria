
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_APPEND_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_APPEND_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorAppendBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Int max_size;

		Params(): BenchmarkParams("Append") {
			Add("max_size", max_size, 512*1024*1024);
		}
	};


	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<Vector>::Type 	MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;


	Allocator* allocator_;
	MapCtr* map_;



public:

	VectorAppendBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~VectorAppendBenchmark() throw() {}

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

		auto i = map_->End();

		ArrayData data(size);

		for (Int c = 0; c < params->max_size / size; c++)
		{
			i.Insert(data);
		}

		result.x() 			= size;

		result.operations() = params->max_size / size;

		allocator_->rollback();
	}

	virtual String GetGraphName()
	{
		return String("Memoria VectorMap Append");
	}
};


}


#endif
