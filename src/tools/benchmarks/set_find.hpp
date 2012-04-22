
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

	SetFindParams(): BenchmarkParams("SetFind")
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

	bool dump_element_count_;

	Int* rd_array_;

public:

	SetBenchmark(bool dump_element_count = true):
		SPBenchmarkTask(new SetFindParams()),
		dump_element_count_(dump_element_count)
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~SetBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();
		return (1024 * (1 << time))/8;
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

		map_ = new MapCtr(*allocator_);

		Iterator i = map_->End();

		for (Int c = 0; c < size; c++)
		{
			Accumulator keys;
			keys[0] = key(c);

			map_->Insert(i, keys);

			i++;
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

		for (Int c = 0; c < params->iterations; c++)
		{
			Iterator i = map_->Find(rd_array_[c]);
			if (i.IsEnd())
			{
				cout<<"MISS!!!"<<endl; // this should't happen
			}


//			if (!map_->Contains(rd_array_[c]))
//			{
//				cout<<"MISS!!!"<<endl; // this should't happen
//			}
		}

		result.x() 			= map_->GetSize();

//		if (dump_element_count_)
//		{
//			result.x() 			= map_->GetSize();
//		}
//		else {
//			result.x() 			= GetBufferSize()/1024;
//		}

		result.operations() = params->iterations;
	}

	virtual String GetGraphName()
	{
		return "Memoria Set<BigInt>";
	}
};


}


#endif
