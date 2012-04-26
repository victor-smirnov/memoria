
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_SCAN_HPP_
#define MEMORIA_BENCHMARKS_SET_SCAN_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetScanBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Params(bool fast): BenchmarkParams(String("SetFind")+(fast? ".Fast":"")) {}
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

	volatile Int result_;

	bool fast_;

public:

	SetScanBenchmark(bool fast = false):
		SPBenchmarkTask(new Params(fast)),
		fast_(fast)
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~SetScanBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * ((1) << time))/8;
	}

	Key key(Int c) const
	{
		return c * 2 + 1;
	}

	virtual void Prepare(ostream& out)
	{
		allocator_ = new Allocator();

		Int size = GetSetSize();

		String resource_name = "allocator."+ToString(size)+".dump";

		if (IsResourceExists(resource_name))
		{
			LoadResource(*allocator_, resource_name);

			map_ = new MapCtr(*allocator_, 1);
		}
		else {
			map_ = new MapCtr(*allocator_, 1, true);

			Iterator i = map_->End();

			for (Int c = 0; c < size; c++)
			{
				Accumulator keys;
				keys[0] = key(c);

				map_->Insert(i, keys);

				i++;
			}

			allocator_->commit();
			StoreResource(*allocator_, resource_name);
		}
	}

	virtual void Release(ostream& out)
	{
		delete map_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		Params* params = GetParameters<Params>();

		BigInt total = 0;

		if (fast_)
		{
			for (Int c = 0; c < params->iterations; )
			{
				for (auto i = map_->begin(); !i.IsEnd() && c < params->iterations; i++, c++)
				{
					total += i.key();
				}
			}
		}
		else {
			for (Int c = 0; c < params->iterations; )
			{
				for (auto i = map_->begin(); !i.IsEnd() && c < params->iterations; i++, c++)
				{
					total += i.GetKey(0);
				}
			}
		}

		result.x() 			= map_->GetSize();

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return String("Memoria Set<BigInt> Scan")+(fast_ ? " Fast": "");
	}
};


}


#endif
