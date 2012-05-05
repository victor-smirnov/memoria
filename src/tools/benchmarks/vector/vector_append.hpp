
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_APPEND_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_APPEND_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorAppendBenchmark: public SPBenchmarkTask {
public:



	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<Vector>::Type 	MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;


	Allocator* 	allocator_;
	MapCtr* 	map_;

	Int 		memory_size;


public:

	VectorAppendBenchmark(StringRef graph_name = "Memoria Vector Append"):
		SPBenchmarkTask("VectorAppend", graph_name), memory_size(128*1024*1024)
	{
		RootCtr::Init();
		MapCtr::Init();

		Add("memory_size", memory_size);
	}

	virtual ~VectorAppendBenchmark() throw() {}

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

		auto i = map_->End();

		BigInt total = 0;

		ArrayData data(size, malloc(size), true);

		while (total < memory_size)
		{
			i.Insert(data);

			total += data.size();
		}

		allocator_->rollback();
	}
};


}


#endif
