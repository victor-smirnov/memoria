
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_APPEND_HPP_
#define MEMORIA_BENCHMARKS_SET_APPEND_HPP_

#include "../benchmarks_inc.hpp"


#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetAppendBenchmark: public SPBenchmarkTask {

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

	SetAppendBenchmark():
		SPBenchmarkTask("Append")
	{
		RootCtr::Init();
		SetCtr::Init();
	}

	virtual ~SetAppendBenchmark() throw() {}

	Key key(Int c) const
	{
		return c * 2 + 1;
	}

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


	virtual void Benchmark(BenchmarkParameters& result, ostream& out)
	{
		Int size = result.x();

		Iterator i = set_->End();

		for (Int c = 0; c < size; c++)
		{
			Accumulator keys;
			keys[0] = key(c);

			set_->Insert(i, keys);

			i++;
		}

		allocator_->rollback();
	}

	virtual String GetGraphName()
	{
		return "Memoria Set<BigInt> Append";
	}
};


}


#endif
