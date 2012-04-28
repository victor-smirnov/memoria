
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_APPEND_BATCH_HPP_
#define MEMORIA_BENCHMARKS_SET_APPEND_BATCH_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetCreateBatchBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {
		Params(): BenchmarkParams("AppendBatch") {}
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

	typedef typename MapCtr::ISubtreeProvider						ISubtreeProvider;
	typedef typename MapCtr::DefaultSubtreeProviderBase				DefaultSubtreeProviderBase;
	typedef typename MapCtr::NonLeafNodeKeyValuePair				NonLeafNodeKeyValuePair;
	typedef typename MapCtr::LeafNodeKeyValuePair 					LeafNodeKeyValuePair;


	class SubtreeProvider: public DefaultSubtreeProviderBase
	{
		typedef DefaultSubtreeProviderBase 		Base;
		typedef typename ISubtreeProvider::Enum Direction;
	public:
		SubtreeProvider(MapCtr* ctr, BigInt total): Base(*ctr, total) {}

		virtual LeafNodeKeyValuePair GetLeafKVPair(Direction direction, BigInt begin)
		{
			Accumulator acc;
			acc[0] = 1;
			return LeafNodeKeyValuePair(acc, Value());
		}
	};


	Allocator* 	allocator_;
	MapCtr* 	map_;

public:

	SetCreateBatchBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~SetCreateBatchBenchmark() throw() {}

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

		allocator_->commit();
	}

	virtual void Release(ostream& out)
	{
		delete map_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		Int size = GetSetSize();

		Iterator i = map_->End();

		SubtreeProvider provider(map_, size);

		map_->InsertSubtree(i, provider);

		allocator_->commit();

		result.x() 			= map_->GetSize();

		result.operations() = size;
	}

	virtual String GetGraphName()
	{
		return String("Memoria Set<BigInt> Batch Append");
	}
};


}


#endif
