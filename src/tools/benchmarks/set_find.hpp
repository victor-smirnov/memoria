
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_FIND_HPP_
#define MEMORIA_BENCHMARKS_SET_FIND_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;

struct SetFindParams: public BenchmarkParams {
	Int iterations;

	SetFindParams(bool fast): BenchmarkParams(String("SetFind")+(fast? ".Fast":""))
	{
		Add("iterations", iterations, 1*1024*1024);
	}
};


class SetBenchmark: public SPBenchmarkTask {

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

	Int* rd_array_;

public:

	SetBenchmark(bool fast):
		SPBenchmarkTask(new SetFindParams(fast)),
		fast_(fast)
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~SetBenchmark() throw() {}

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
		SetFindParams* params = GetParameters<SetFindParams>();

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

		rd_array_ = new Int[params->iterations];
		for (Int c = 0; c < params->iterations; c++)
		{
			rd_array_[c] = key(GetRandom(size));
		}
	}

	virtual void Release(ostream& out)
	{
		delete map_;
		delete allocator_;
		delete[] rd_array_;
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		PSetFindParams* params = GetParameters<PSetFindParams>();

		if (fast_)
		{
			for (Int c = 0; c < params->iterations; c++)
			{
				if (!map_->Contains1(rd_array_[c]))
				{
					cout<<"MISS!!!"<<endl; // this should't happen
				}
			}
		}
		else
		{
			for (Int c = 0; c < params->iterations; c++)
			{
				if (!map_->Contains(rd_array_[c]))
				{
					cout<<"MISS!!!"<<endl; // this should't happen
				}
			}
		}

		result.x() 			= map_->GetSize();

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return String("Memoria Set<BigInt>") + (fast_ ? " (fast)" : "");
	}
};


}


#endif
