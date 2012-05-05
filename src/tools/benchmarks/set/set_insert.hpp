
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_INSERT_HPP_
#define MEMORIA_BENCHMARKS_SET_INSERT_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetInsertBenchmark: public SPBenchmarkTask {

	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<Set1>::Type 		SetCtr;
	typedef typename SetCtr::Iterator								Iterator;
	typedef typename SetCtr::ID										ID;
	typedef typename SetCtr::Accumulator							Accumulator;


	typedef typename SetCtr::Key									Key;
	typedef typename SetCtr::Value									Value;


	Allocator* 	allocator_;
	SetCtr* 	set_;



public:

	SetInsertBenchmark(StringRef graph_name = "Memoria Set<BigInt> Random Insert"):
		SPBenchmarkTask("InsertRandom", graph_name)
	{
		RootCtr::Init();
		SetCtr::Init();
	}

	virtual ~SetInsertBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ = new Allocator();

		set_ = new SetCtr(*allocator_, 1, true);
	}

	virtual void Release(ostream& out)
	{
		delete set_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();

		params.operations() = size;

		for (Int c = 0; c < size; c++)
		{
			auto i = set_->Find(GetRandom(size));

			Accumulator keys;
			keys[0] = 1;

			set_->InsertRaw(i, keys);

			keys[0] = 0;
			i++;

			if (i.IsNotEnd())
			{
				i.UpdateUp(keys);
			}
		}
	}
};


}


#endif
