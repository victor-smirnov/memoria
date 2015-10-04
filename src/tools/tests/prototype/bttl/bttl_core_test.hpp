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
    	MEMORIA_ADD_TEST_PARAM(iterations);
    	MEMORIA_ADD_TEST_PARAM(level_limit);
    	MEMORIA_ADD_TEST_PARAM(last_level_limit);

    	MEMORIA_ADD_TEST(testDetProvider);
    	MEMORIA_ADD_TEST(testRngProvider);
    }

    virtual ~BTTLCoreTest() throw () {}

    void createAllocator(AllocatorSPtr& allocator) {
    	allocator = std::make_shared<Allocator>();
    }

    void testDetProvider()
    {
    	for (Int i = 0; i < iterations; i++)
    	{
    		this->out()<<"Iteration "<<(i + 1)<<endl;

    		{
    			Ctr ctr = this->createCtr();

    			auto shape = this->sampleTreeShape(level_limit, last_level_limit, size);

    			this->out()<<"shape: "<<shape<<endl;

    			DetInputProvider provider(shape);

    			testProvider(ctr, provider);
    		}

    		createAllocator(this->allocator_);
    	}
    }

    void testRngProvider()
    {
    	for (Int i = 0; i < iterations; i++)
    	{
    		this->out()<<"Iteration "<<(i + 1)<<endl;
    		{
    			Ctr ctr = this->createCtr();

    			auto shape = this->sampleTreeShape(level_limit, last_level_limit, size);

    			this->out()<<"shape: "<<shape<<endl;

    			RngInputProvider provider(shape, int_generator);

    			testProvider(ctr, provider);
    		}

    		createAllocator(this->allocator_);
    	}
    }









    template <typename Provider>
    void testProvider(Ctr& ctr, Provider& provider)
    {
    	this->fillCtr(ctr, provider);

    	this->checkExtents(ctr);
    	this->checkRanks(ctr);

    	this->out()<<endl;
    }
};

}

#endif
