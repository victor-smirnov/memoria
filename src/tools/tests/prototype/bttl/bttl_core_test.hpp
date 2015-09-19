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

	Int rows 		= 1000000;
	Int cols		= 10;
	Int data_size	= 100;

public:

    BTTLCoreTest(String name):
    	Base(name)
    {
    	MEMORIA_ADD_TEST(testDetProvider);
    	MEMORIA_ADD_TEST(testRngProvider);
    }

    virtual ~BTTLCoreTest() throw () {}


    void createAllocator(AllocatorSPtr& allocator) {
    	allocator = std::make_shared<Allocator>();
    }


    void testDetProvider()
    {
    	using Provider = DetInputProvider;

    	Ctr ctr = this->createCtr();

    	Provider provider(ctr, CtrSizesT({rows, cols, data_size}), 0);

    	testProvider(ctr, provider);
    }

    void testRngProvider()
    {
    	using Provider = RngInputProvider;

    	Ctr ctr = this->createCtr();

    	Provider provider(ctr, CtrSizesT({rows, cols, data_size}), 0, int_generator);

    	testProvider(ctr, provider);
    }



    template <typename Provider>
    void testProvider(Ctr& ctr, Provider& provider)
    {
    	auto iter = ctr.seek(0);

    	long t0 = getTimeInMillis();

    	ctr.insertData(iter.leaf(), CtrSizesT(), provider);

    	long t1 = getTimeInMillis();

    	cout<<FormatTime(t1 - t0)<<endl;

    	auto sizes = ctr.sizes();

    	AssertEQ(MA_SRC, provider.consumed(), sizes);

    	auto ctr_totals = ctr.total_counts();

    	cout<<"Totals: "<<ctr_totals<<" "<<sizes<<endl;

    	AssertEQ(MA_SRC, ctr_totals, sizes);

    	this->allocator()->commit();

    	ctr.seek(0).split();
    }
};

}

#endif
