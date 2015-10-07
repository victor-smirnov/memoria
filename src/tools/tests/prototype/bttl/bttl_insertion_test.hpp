// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_INSERTION_TEST_HPP_
#define MEMORIA_TESTS_BTTL_INSERTION_TEST_HPP_

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
class BTTLInsertionTest;

template <
	Int Levels,
	PackedSizeType SizeType,
	typename AllocatorT,
	typename ProfileT
>
class BTTLInsertionTest<BTTLTestCtr<Levels, SizeType>, AllocatorT, ProfileT>: public BTTLTestBase<BTTLTestCtr<Levels, SizeType>, AllocatorT, ProfileT> {

	using CtrName = BTTLTestCtr<Levels, SizeType>;

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

	Int level  = -1;


	CtrSizesT 	shape_;
	CtrSizesT	insertion_pos_;
	BigInt		ctr_name_;

	Int level_;

public:

    BTTLInsertionTest(String name):
    	Base(name)
    {

    	MEMORIA_ADD_TEST_PARAM(level);

    	MEMORIA_ADD_TEST_PARAM(shape_)->state();
    	MEMORIA_ADD_TEST_PARAM(insertion_pos_)->state();
    	MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
    	MEMORIA_ADD_TEST_PARAM(level_)->state();

    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsert, replayInsert);
    }

    virtual ~BTTLInsertionTest() throw () {}

    virtual void smokeCoverage(Int scale)
    {
    	this->size 			= 10000;
    	this->iterations	= 1 * scale;
    }

    virtual void smallCoverage(Int scale)
    {
    	this->size 			= 100000 ;
    	this->iterations	= 3 * scale;
    }

    virtual void normalCoverage(Int scale)
    {
    	this->size 			= 100000;
    	this->iterations	= 50 * scale;
    }

    virtual void largeCoverage(Int scale)
    {
    	this->size 			= 1000000;
    	this->iterations	= 10 * scale;
    }

    void createAllocator(AllocatorSPtr& allocator)
    {
    	allocator = std::make_shared<Allocator>();
    	allocator->mem_limit() = this->hard_memlimit_;
    }




    CtrSizesT testInsertionStep(Ctr& ctr)
    {
    	auto sizes = ctr.sizes();

    	auto iter = ctr.seek(insertion_pos_[0]);

    	for (Int s = 1; s <= level_; s++) {
    		iter.toData(insertion_pos_[s]);
    	}

    	for (Int s = 0; s < level_; s++) {
    		shape_[s] = 0;
    	}

    	this->out()<<"Insert "<<shape_<<" data at: "<<insertion_pos_<<endl;

    	DetInputProvider provider(shape_, level_);

    	auto totals = ctr._insert(iter, provider);

    	auto new_sizes = ctr.sizes();
    	AssertEQ(MA_SRC, new_sizes, sizes + totals);

    	this->checkAllocator(MA_SRC, "Insert: Container Check Failed");

    	this->checkExtents(ctr);

    	auto t0 = getTimeInMillis();

    	if (level_ == 0)
    	{
    		for (CtrSizeT c = insertion_pos_[level_]; c < insertion_pos_[level_] + totals[level_]; c++)
    		{
    			this->checkSubtree(ctr, c);
    		}
    	}
    	else
    	{
    		auto iter = ctr.seek(insertion_pos_[0]);

    		for (Int s = 1; s <= level_; s++)
    		{
    			iter.toData(insertion_pos_[s]);
    		}

    		this->checkSubtree(iter, totals[level_]);
    	}

    	this->out()<<"Check subtree is done in "<<FormatTime(getTimeInMillis() - t0)<<endl;

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

    CtrSizeT sampleSize(Int iteration, CtrSizeT size)
    {
    	if (iteration % 3 == 0)
    	{
    		return this->getRandom(size);
    	}
    	else if (iteration % 3 == 1) {
    		return size;
    	}
    	else {
    		return 0;
    	}
    }

    CtrSizeT sampleSizeNZ(Int iteration, CtrSizeT size)
    {
    	if (iteration % 2 == 0)
    	{
    		return size > 0 ? (this->getRandom(size - 1) + 1) : 0;
    	}
    	else
    	{
    		return size;
    	}
    }

    CtrSizeT sampleSizeNZ(Int level, Int iteration, CtrSizeT size)
    {
    	if (iteration % 2 == 0)
    	{
    		return size > 0 ? (this->getRandom(size - 1) + 1) : 0;
    	}
    	else
    	{
    		return size;
    	}
    }

    void testInsert() {

    	if (level == -1)
    	{
    		for (Int c = 0; c < Levels - 1; c++)
    		{
    			testInsertForLevel(c);
    			createAllocator(this->allocator());
    			this->out()<<endl;
    		}
    	}
    	else {
    		testInsertForLevel(level);
    	}
    }

    void testInsertForLevel(Int level)
    {
    	this->out()<<"Test for level: "<<level<<endl;

    	Ctr ctr = this->createCtr();
        this->ctr_name_ = ctr.name();

        this->commit();

        try {
            for (Int c = 0; c < this->iterations && this->checkSoftMemLimit(); c++)
            {
            	this->out()<<"Iteration: "<<c<<endl;

            	auto sizes = ctr.sizes();

            	insertion_pos_ = CtrSizesT(-1);

        		insertion_pos_[0] = sampleSize(c, sizes[0]);

        		CtrSizesT path_sizes;
        		path_sizes[0] = sizes[0];


            	auto iter = ctr.seek(insertion_pos_[0]);
            	level_ = 0;

            	for (Int s = 1; s <= level; s++)
            	{
            		if (insertion_pos_[s - 1] < path_sizes[s - 1])
            		{
            			auto local_size = iter.size();

            			if (local_size > 0)
            			{
            				if (insertion_pos_[s - 1] > 0) {
            					insertion_pos_[s] = sampleSize(c, local_size);
            				}
            				else {
            					insertion_pos_[s] = sampleSizeNZ(c, local_size);
            				}

            				iter.toData(insertion_pos_[s]);
            				level_ = s;

            				path_sizes[s] = local_size;
            			}
            			else {
            				break;
            			}
            		}
            		else {
            			break;
            		}
            	}

            	shape_ = this->sampleTreeShape();

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
