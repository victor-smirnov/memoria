
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_INSERT_HPP_
#define MEMORIA_BENCHMARKS_SET_INSERT_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetInsertBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Params(): BenchmarkParams("Insert") {}
	};


	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<Set1>::Type 		MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;
	typedef typename MapCtr::Accumulator							Accumulator;


	typedef typename MapCtr::Key									Key;
	typedef typename MapCtr::Value									Value;


	Allocator* allocator_;
	MapCtr* map_;



public:

	SetInsertBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~SetInsertBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * ((1) << time));
	}

	Key key(Int c) const
	{
		return c * 2 + 1;
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

		for (Int c = 0; c < size; c++)
		{
			auto i = map_->Find(GetRandom(size));

			Accumulator keys;
			keys[0] = 1;

			map_->InsertRaw(i, keys);

			keys[0] = 0;
			i++;

			if (i.IsNotEnd())
			{
				i.UpdateUp(keys);
			}
		}

		result.x() 			= map_->GetSize();

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return String("Memoria Set<BigInt> Insert");
	}
};


}


#endif
