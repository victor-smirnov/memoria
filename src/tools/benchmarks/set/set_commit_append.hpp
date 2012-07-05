
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_COMMIT_APPEND_HPP_
#define MEMORIA_BENCHMARKS_SET_COMMIT_APPEND_HPP_

#include "../benchmarks_inc.hpp"


#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class setCommitAppendBenchmark: public SPBenchmarkTask {

	public:
	Int max_size;


	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<set1>::Type 		setCtr;
	typedef typename setCtr::Iterator								Iterator;
	typedef typename setCtr::ID										ID;
	typedef typename setCtr::Accumulator							Accumulator;


	typedef typename setCtr::Key									Key;
	typedef typename setCtr::Value									Value;

	Allocator* 	allocator_;
	setCtr* 	set_;

public:

	setCommitAppendBenchmark(StringRef name):
		SPBenchmarkTask(name), max_size(1*1024*1024)
	{
		Add("max_size", max_size);

		RootCtr::Init();
		setCtr::Init();
	}

	virtual ~setCommitAppendBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ = new Allocator();

		set_ = new setCtr(*allocator_, 1, true);
	}

	virtual void Release(ostream& out)
	{
		delete set_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();

		auto i = set_->End();

		params.operations() = this->max_size;

		for (Int c = 0; c < this->max_size; c++)
		{
			Accumulator keys;
			keys[0] = 1;

			set_->InsertRaw(i, keys);

			if (c % size == 0)
			{
				allocator_->commit();
			}
		}

		allocator_->commit();
	}
};


}


#endif
