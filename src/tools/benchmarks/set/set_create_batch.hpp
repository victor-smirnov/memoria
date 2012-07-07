
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_APPEND_BATCH_HPP_
#define MEMORIA_BENCHMARKS_SET_APPEND_BATCH_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class setCreateBatchBenchmark: public SPBenchmarkTask {

	typedef SPBenchmarkTask Base;

	typedef typename Base::Allocator 	Allocator;
	typedef typename Base::Profile 		Profile;

	typedef typename SmallCtrTypeFactory::Factory<Root>::Type 		RootCtr;
	typedef typename SmallCtrTypeFactory::Factory<set1>::Type 		SetCtr;
	typedef typename SetCtr::Iterator								Iterator;
	typedef typename SetCtr::ID										ID;
	typedef typename SetCtr::Accumulator							Accumulator;


	typedef typename SetCtr::Key									Key;
	typedef typename SetCtr::Value									Value;

	typedef typename SetCtr::ISubtreeProvider						ISubtreeProvider;
	typedef typename SetCtr::DefaultSubtreeProviderBase				DefaultSubtreeProviderBase;
	typedef typename SetCtr::NonLeafNodeKeyValuePair				NonLeafNodeKeyValuePair;
	typedef typename SetCtr::LeafNodeKeyValuePair 					LeafNodeKeyValuePair;


	class SubtreeProvider: public DefaultSubtreeProviderBase
	{
		typedef DefaultSubtreeProviderBase 		Base;
		typedef typename ISubtreeProvider::Enum Direction;
	public:
		SubtreeProvider(SetCtr* ctr, BigInt total): Base(*ctr, total) {}

		virtual LeafNodeKeyValuePair getLeafKVPair(Direction direction, BigInt begin)
		{
			Accumulator acc;
			acc[0] = 1;
			return LeafNodeKeyValuePair(acc, Value());
		}
	};


	Allocator* 	allocator_;
	SetCtr* 	set_;

public:

	setCreateBatchBenchmark(StringRef name):
		SPBenchmarkTask(name)
	{
		RootCtr::initMetadata();
		SetCtr::initMetadata();
	}

	virtual ~setCreateBatchBenchmark() throw() {}

	virtual void Prepare(BenchmarkParameters& params, ostream& out)
	{
		allocator_ 	= new Allocator();
		set_ 		= new SetCtr(allocator_, 1, true);

		allocator_->commit();
	}

	virtual void release(ostream& out)
	{
		delete set_;
		delete allocator_;
	}


	virtual void Benchmark(BenchmarkParameters& params, ostream& out)
	{
		Int size = params.x();

		params.operations() = size;

		Iterator i = set_->End();

		SubtreeProvider provider(set_, size);

		set_->insertSubtree(i, provider);

		allocator_->commit();
	}
};


}


#endif
