// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_CORE_TEST_HPP_
#define MEMORIA_TESTS_BTTL_CORE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../../shared/bttl_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

template <
    typename CtrName,
	typename AllocatorT 	= SmallInMemAllocator,
	typename ProfileT		= SmallProfile<>
>
class BTTLCoreTest: public BTTLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base 	 = BTTLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTTLCoreTest<CtrName, AllocatorT, ProfileT>;

    using Allocator 	= typename Base::Allocator;
    using AllocatorSPtr = typename Base::AllocatorSPtr;
    using Ctr 			= typename Base::Ctr;

    using DetInputProvider  	= bttl::DeterministicDataInputProvider<Ctr>;
    using RngInputProvider  	= bttl::RandomDataInputProvider<Ctr, RngInt>;

    using Rng 			 = typename RngInputProvider::Rng;

    using CtrSizesT 	 = typename Ctr::Types::Position;

    static const Int Streams = Ctr::Types::Streams;

	BigInt size 			= 1000000;
	Int level_limit 		= 1000;
	int last_level_limit 	= 100;

	Int iterations 			= 5;


public:

    BTTLCoreTest(String name):
    	Base(name)
    {
    	MEMORIA_ADD_TEST_PARAM(size);

    	MEMORIA_ADD_TEST(testDetProvider);
    	MEMORIA_ADD_TEST(testRngProvider);
    }

    virtual ~BTTLCoreTest() throw () {}

    CtrSizesT sampleTreeShape()
    {
    	CtrSizesT shape;

    	CtrSizesT limits(level_limit);
    	limits[Streams - 1] = last_level_limit;

    	while(shape[0] == 0)
    	{
    		BigInt resource = size;

    		for (Int c = Streams - 1; c > 0; c--)
    		{
    			Int level_size = getRandom(limits[c]) + 1;

    			shape[c] = level_size;

    			resource = resource / level_size;
    		}

    		shape[0] = resource;
    	}

    	return shape;
    }

    void createAllocator(AllocatorSPtr& allocator) {
    	allocator = std::make_shared<Allocator>();
    }



    void testDetProvider()
    {
    	using Provider = DetInputProvider;

    	for (Int i = 0; i < iterations; i++)
    	{
    		this->out()<<"Iteration "<<(i + 1)<<endl;

    		{
    			Ctr ctr = this->createCtr();

    			auto shape = sampleTreeShape();

    			this->out()<<"shape: "<<shape<<endl;

    			Provider provider(ctr, shape, 0);

    			testProvider(ctr, provider);
    		}

    		this->storeAllocator("core.dump");

    		createAllocator(this->allocator_);
    	}
    }

    void testRngProvider()
    {
    	using Provider = RngInputProvider;

    	for (Int i = 0; i < iterations; i++)
    	{
    		this->out()<<"Iteration "<<(i + 1)<<endl;
    		{
    			Ctr ctr = this->createCtr();

    			auto shape = sampleTreeShape();

    			this->out()<<"shape: "<<shape<<endl;

    			Provider provider(ctr, shape, 0, int_generator);

    			testProvider(ctr, provider);
    		}

    		createAllocator(this->allocator_);
    	}
    }


    void checkExtents(Ctr& ctr)
    {
    	auto i = ctr.seek(0);

    	CtrSizesT extent;

    	long t2 = getTimeInMillis();

    	do
    	{
    		auto current_extent = i.leaf_extent();

    		AssertEQ(MA_SRC, current_extent, extent);

    		extent += ctr.node_extents(i.leaf());
    	}
    	while(i.nextLeaf());

    	long t3 = getTimeInMillis();

    	this->out()<<"Extent verification time: "<<FormatTime(t3 - t2)<<endl;
    }


    void checkRanks(Ctr& ctr)
    {
    	auto i = ctr.seek(0);

    	CtrSizesT extents;

    	long t2 = getTimeInMillis();

    	do
    	{
    		auto sizes = i.leaf_sizes();

    		auto total_leaf_rank = sizes.sum();

    		typename Ctr::Types::LeafPrefixRanks prefix_ranks;

    		ctr.compute_leaf_prefixes(i.leaf(), extents, prefix_ranks);

    		auto total_ranks = ctr.leaf_rank(i.leaf(), sizes, prefix_ranks, sizes.sum());

    		AssertEQ(MA_SRC, total_ranks, sizes);

    		for (Int c = 0; c < total_leaf_rank; )
    		{
    			auto ranks = ctr.leaf_rank(i.leaf(), sizes, prefix_ranks, c);

    			AssertEQ(MA_SRC, ranks.sum(), c);

    			c += getRandom(100) + 1;
    		}

    		extents += ctr.node_extents(i.leaf());
    	}
    	while(i.nextLeaf());

    	long t3 = getTimeInMillis();

    	this->out()<<"Rank verification time: "<<FormatTime(t3 - t2)<<endl;

    }



    template <typename Provider>
    void testProvider(Ctr& ctr, Provider& provider)
    {
    	this->fillCtr(ctr, provider);

    	checkExtents(ctr);
    	checkRanks(ctr);

    	this->out()<<endl;
    }
};

}

#endif
