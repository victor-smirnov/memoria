// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_INSERTION_TEST_HPP_
#define MEMORIA_TESTS_BTTL_INSERTION_TEST_HPP_

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
class BTTLInsertionTest: public BTTLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base 	 = BTTLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTTLInsertionTest<CtrName, AllocatorT, ProfileT>;

    using Allocator 	= typename Base::Allocator;
    using AllocatorSPtr = typename Base::AllocatorSPtr;
    using Ctr 			= typename Base::Ctr;

    using DetInputProvider  	= bttl::DeterministicDataInputProvider<Ctr>;
    using RngInputProvider  	= bttl::RandomDataInputProvider<Ctr, RngInt>;

    using Rng 			 = typename RngInputProvider::Rng;

    using CtrSizesT 	 = typename Ctr::Types::Position;
    using CtrSizeT 	 	 = typename Ctr::Types::CtrSizeT;

    static const Int Streams = Ctr::Types::Streams;

    Int size 				= 1000000;
	Int level_limit 		= 1000;
	int last_level_limit 	= 100;

	Int iterations 			= 5;


	CtrSizesT 	shape_;
	CtrSizeT	insertion_pos_;
	BigInt		ctr_name_;

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

    BTTLInsertionTest(String name):
    	Base(name)
    {
    	MEMORIA_ADD_TEST_PARAM(size);
    	MEMORIA_ADD_TEST_PARAM(iterations);
    	MEMORIA_ADD_TEST_PARAM(level_limit);
    	MEMORIA_ADD_TEST_PARAM(last_level_limit);

    	MEMORIA_ADD_TEST_PARAM(shape_)->state();
    	MEMORIA_ADD_TEST_PARAM(insertion_pos_)->state();
    	MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();

    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsert, replayInsert);
    }

    virtual ~BTTLInsertionTest() throw () {}



    void createAllocator(AllocatorSPtr& allocator)
    {
    	allocator = std::make_shared<Allocator>();
    }




    CtrSizesT testInsertionStep(Ctr& ctr)
    {
    	DetInputProvider provider(shape_);

    	auto sizes = ctr.sizes();

    	this->out()<<"Insert "<<shape_<<" data at: "<<insertion_pos_<<endl;
    	auto iter = ctr.seek(insertion_pos_);

    	auto totals = ctr._insert(iter, provider);

    	this->checkAllocator(MA_SRC, "Insert: Container Check Failed");
    	this->checkExtents(ctr);
//    	this->checkRanks(ctr);

    	auto new_sizes = ctr.sizes();

    	AssertEQ(MA_SRC, new_sizes, sizes + totals);

    	for (CtrSizeT c = insertion_pos_; c < insertion_pos_ + totals[0]; c++)
    	{
    		this->checkSubtree(ctr, c);
    	}

    	return totals;

    }

    void replayInsert()
    {
    	this->out()<<"Replay!"<<endl;

    	this->loadAllocator(this->dump_name_);

    	Ctr ctr = this->findCtr(ctr_name_);

    	try {
    		testInsertionStep(ctr);
    	}
    	catch (...) {
    		this->commit();
    		this->storeAllocator(this->dump_name_+"-repl");
    		throw;
    	}
    }

    template <typename Provider>
    CtrSizesT testProvider(Ctr& ctr, Provider& provider)
    {
    	auto sizes = ctr.sizes();

    	CtrSizeT pos = getRandom(sizes[0] + 1);

    	this->out()<<"Insert "<<shape_<<" data at: "<<pos<<endl;
    	auto iter = ctr.seek(pos);

    	auto totals = ctr._insert(iter, provider);

    	this->checkAllocator(MA_SRC, "");

    	auto new_sizes = ctr.sizes();

    	AssertEQ(MA_SRC, new_sizes, sizes + totals);

    	for (CtrSizeT c = pos; c < totals[0]; c++)
    	{
    		this->checkSubtree(ctr, c);
    	}

    	return totals;
    }

    void testInsert()
    {
    	Ctr ctr = this->createCtr();
        this->ctr_name_ = ctr.name();

        this->commit();

        try {
            for (Int c = 0; c < iterations; c++)
            {
            	this->out()<<"Iteration: "<<c<<endl;

            	auto sizes = ctr.sizes();

            	insertion_pos_ = getRandom(sizes[0] + 1);
//            	insertion_pos_ = 0;

//            	insertion_pos_ = sizes[0];

            	shape_ = this->sampleTreeShape(level_limit, last_level_limit, size);

            	testInsertionStep(ctr);

                this->out()<<"Sizes: "<<ctr.sizes()<<endl<<endl;

                this->commit();
            }
        }
        catch (...) {
        	this->dump_name_ = this->Store();
            throw;
        }
    }
};

}

#endif
