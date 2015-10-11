// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_ITER_TEST_HPP_
#define MEMORIA_TESTS_BTTL_ITER_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "bttl_test_base.hpp"

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

    using CtrSizeT 	 	 = typename Ctr::Types::CtrSizeT;
    using CtrSizesT 	 = typename Ctr::Types::Position;

    static const Int Streams = Ctr::Types::Streams;



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
    	MEMORIA_ADD_TEST(testDetProvider);
    	MEMORIA_ADD_TEST(testRngProvider);
    }

    virtual ~BTTLIterTest() throw () {}



    void createAllocator(AllocatorSPtr& allocator) {
    	allocator = std::make_shared<Allocator>();
    	allocator->mem_limit() = this->hard_memlimit_;
    }



    void testDetProvider()
    {
    	for (Int i = 0; i < this->iterations; i++)
    	{
    		this->out()<<"Iteration "<<(i + 1)<<endl;

    		{
    			Ctr ctr = this->createCtr();

    			auto shape = this->sampleTreeShape();

    			this->out()<<"shape: "<<shape<<endl;

    			DetInputProvider provider(shape);

    			auto totals = this->fillCtr(ctr, provider);

//    			checkScan(ctr, shape);
//    			this->checkSubtree(ctr, path);

    			for (CtrSizeT r = 0; r < totals[0]; r++)
    			{
    				CtrSizesT path(-1);
    				path[0] = r;

    				this->checkSubtree(ctr, path);
    			}

    			this->out()<<endl;
    		}

    		createAllocator(this->allocator_);
    	}
    }

    void testRngProvider()
    {
    	for (Int i = 0; i < this->iterations; i++)
    	{
    		this->out()<<"Iteration "<<(i + 1)<<endl;

    		{
    			Ctr ctr = this->createCtr();

    			auto shape = this->sampleTreeShape();

    			this->out()<<"shape: "<<shape<<endl;

    			RngInputProvider provider(shape, this->getIntTestGenerator());

    			auto totals = this->fillCtr(ctr, provider);

    			for (CtrSizeT r = 0; r < totals[0]; r++)
    			{
    				CtrSizesT path(-1);
    				path[0] = r;

    				this->checkSubtree(ctr, path);
    			}

    			this->out()<<endl;
    		}

    		createAllocator(this->allocator_);
    	}
    }


//    void checkScan(Ctr& ctr, CtrSizesT sizes)
//    {
//    	auto i = ctr.seek(0);
//
//    	CtrSizesT extent;
//
//    	long t2 = getTimeInMillis();
//
//    	auto rows = sizes[0];
//    	auto cols = sizes[1];
//
//    	auto iter = ctr.seek(0);
//    	for (Int r = 0; r < rows; r++)
//    	{
//    		AssertEQ(MA_SRC, iter.pos(), r);
//    		AssertEQ(MA_SRC, iter.cache().abs_pos()[0], r);
//    		AssertEQ(MA_SRC, iter.size(), rows);
//
//    		iter.toData();
//    		iter.checkPrefix();
//
//    		for (Int c = 0; c < cols; c++)
//    		{
//    			AssertEQ(MA_SRC, iter.pos(), c);
//    			AssertEQ(MA_SRC, iter.size(), cols);
//
//    			iter.toData();
//
//    			ScanFn scan_fn((c + 1) % 256);
//    			auto scanned = iter.template scan<IntList<2>>(scan_fn);
//
//    			AssertEQ(MA_SRC, scanned, iter.size());
//
//    			AssertTrue(MA_SRC, iter.isSEnd());
//
//    			iter.toIndex();
//    			iter.skipFw(1);
//    		}
//
//
//    		iter.toIndex();
//
//    		iter.skipFw(1);
//    	}
//
//    	long t3 = getTimeInMillis();
//
//    	this->out()<<"Scan time: "<<FormatTime(t3 - t2)<<endl;
//    }
};

}

#endif
