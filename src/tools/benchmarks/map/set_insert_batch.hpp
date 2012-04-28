
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_INSERT_BATCH_HPP_
#define MEMORIA_BENCHMARKS_SET_INSERT_BATCH_HPP_

#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/pmap/packed_sum_tree.hpp>

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetInsertBatchBenchmark: public SPBenchmarkTask {

	struct Params: public BenchmarkParams {

		Int max_size;

		Params(): BenchmarkParams("InsertBatch")
		{
			Add("max_size", max_size, 32*1024*1024);
		}
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


	Allocator* allocator_;
	MapCtr* map_;



public:

	SetInsertBatchBenchmark():
		SPBenchmarkTask(new Params())
	{
		RootCtr::Init();
		MapCtr::Init();
	}

	virtual ~SetInsertBatchBenchmark() throw() {}

	Int GetSetSize() const
	{
		Int time = this->GetIteration();

		Int pow = 1;
		for (Int c = 0; c < time; c++) pow *= 10;

		return pow;
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

	void CheckIterator(Iterator i, Int target)
	{
		Int c;
		for (c = 0; !i.IsBegin(); i--, c++);

		if (c != target) {
			cout<<"Not equals: "<<c<<" "<<target<<endl;
		}
	}

	virtual void Benchmark(BenchmarkResult& result, ostream& out)
	{
		Params* params = GetParameters<Params>();

		Int size = GetSetSize();

		SubtreeProvider provider(map_, size);

		Int map_size = 0;

		for (Int c = 0; c < params->max_size / size; c++)
		{
			Int pos = GetRandom(map_size - 1) + 1;
			auto i = map_size == 0? map_->End() : map_->Find(pos);

//			CheckIterator(i, pos);

			map_->InsertSubtree(i, provider);

			map_size += size;
		}

		result.x() 			= size;

		result.operations() = params->iterations;

		allocator_->commit();

		StoreAllocator(*allocator_, "insert-batch.dump");
	}

	virtual String GetGraphName()
	{
		return String("Memoria Set<BigInt> Insert");
	}
};


}


#endif
