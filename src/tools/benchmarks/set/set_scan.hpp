
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_SCAN_HPP_
#define MEMORIA_BENCHMARKS_SET_SCAN_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class setScanBenchmark: public SPBenchmarkTask {

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

	Int 		result_;

public:

	setScanBenchmark(StringRef name):
		SPBenchmarkTask(name)
	{
		RootCtr::Init();
		setCtr::Init();
	}

	virtual ~setScanBenchmark() throw() {}

	Key key(Int c) const
	{
		return c * 2 + 1;
	}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ = new Allocator();

		Int size = params.x();

		String resource_name = "allocator."+ToString(size)+".dump";

		if (IsResourceExists(resource_name))
		{
			LoadResource(*allocator_, resource_name);

			set_ = new setCtr(*allocator_, 1);
		}
		else {
			set_ = new setCtr(*allocator_, 1, true);

			Iterator i = set_->End();

			for (Int c = 0; c < size; c++)
			{
				Accumulator keys;
				keys[0] = key(c);

				set_->insert(i, keys);

				i++;
			}

			allocator_->commit();

			StoreResource(*allocator_, resource_name);
		}
	}

	virtual void Release(ostream& out)
	{
		delete set_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		BigInt total = 0;

		for (Int c = 0; c < params.operations(); )
		{
			for (auto i = set_->begin(); !i.IsEnd() && c < params.operations(); i++, c++)
			{
				total += i.getKey(0);
			}
		}
	}
};


}


#endif
