
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_COMMIT_RANDOM_HPP_
#define MEMORIA_BENCHMARKS_SET_COMMIT_RANDOM_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetCommitRandomBenchmark: public SPBenchmarkTask {
public:

	Int max_size;


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

	SetCommitRandomBenchmark(StringRef graph_name = "Memoria Set<BigInt> Random Commit Rate"):
		SPBenchmarkTask("CommitRandom", graph_name), max_size(16*1024*1024)
	{
		RootCtr::Init();
		SetCtr::Init();

		Add("max_size", max_size);
	}

	virtual ~SetCommitRandomBenchmark() throw() {}

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

		params.operations() = this->max_size;

		for (Int c = 0; c < this->max_size; c++)
		{
			auto i = c == 0? set_->End() : set_->Find(GetRandom(c));

			Accumulator keys;
			keys[0] = 1;

			set_->InsertRaw(i, keys);

			keys[0] = 0;
			i++;

			if (i.IsNotEnd())
			{
				i.UpdateUp(keys);
			}

			if (c % size == 0)
			{
				allocator_->commit();
			}
		}
	}
};


}


#endif
