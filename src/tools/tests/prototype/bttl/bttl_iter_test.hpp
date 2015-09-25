// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_ITER_TEST_HPP_
#define MEMORIA_TESTS_BTTL_ITER_TEST_HPP_

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
class BTTLIterTest: public BTTLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base 	 = BTTLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTTLIterTest<CtrName, AllocatorT, ProfileT>;

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


	struct ScanFn {
		Byte expected_;

		ScanFn(Byte expected): expected_(expected) {}

		template <typename Stream>
		void operator()(const Stream* obj, Int start, Int end)
		{
			for (Int c = start; c < end; c++)
			{
				MEMORIA_ASSERT(obj->value(c), ==, expected_);
			}
		}
	};

public:

    BTTLIterTest(String name):
    	Base(name)
    {
    	MEMORIA_ADD_TEST_PARAM(size);
    	MEMORIA_ADD_TEST_PARAM(iterations);
    	MEMORIA_ADD_TEST_PARAM(level_limit);
    	MEMORIA_ADD_TEST_PARAM(last_level_limit);

    	MEMORIA_ADD_TEST(testDetProvider);
    }

    virtual ~BTTLIterTest() throw () {}



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

    			using Provider = bttl::StreamingCtrInputProvider<Ctr, DetInputProvider>;

    			DetInputProvider p(shape);

    			Provider provider(ctr, p);

    			testProvider(ctr, provider, shape);
    		}

    		createAllocator(this->allocator_);
    	}
    }


    void checkScan(Ctr& ctr, CtrSizesT sizes)
    {
    	auto i = ctr.seek(0);

    	CtrSizesT extent;

    	long t2 = getTimeInMillis();

    	auto rows = sizes[0];
    	auto cols = sizes[1];

    	auto iter = ctr.seek(0);
    	for (Int r = 0; r < rows; r++)
    	{
    		AssertEQ(MA_SRC, iter.pos(), r);
    		AssertEQ(MA_SRC, iter.cache().abs_pos()[0], r);
    		AssertEQ(MA_SRC, iter.size(), rows);

    		iter.toData();
    		iter.checkPrefix();

    		for (Int c = 0; c < cols; c++)
    		{
    			AssertEQ(MA_SRC, iter.pos(), c);
    			AssertEQ(MA_SRC, iter.size(), cols);

    			iter.toData();

    			ScanFn scan_fn((c + 1) % 256);
    			auto scanned = iter.template scan<IntList<2>>(scan_fn);

    			AssertEQ(MA_SRC, scanned, iter.size());

    			AssertTrue(MA_SRC, iter.isSEnd());

    			iter.toIndex();
    			iter.skipFw(1);
    		}


    		iter.toIndex();

    		iter.skipFw(1);
    	}

    	long t3 = getTimeInMillis();

    	this->out()<<"Scan time: "<<FormatTime(t3 - t2)<<endl;
    }



    template <typename Provider>
    void testProvider(Ctr& ctr, Provider& provider, CtrSizesT shape)
    {
    	this->fillCtr(ctr, provider);

    	checkScan(ctr, shape);

    	this->out()<<endl;
    }
};

}

#endif
