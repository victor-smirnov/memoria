
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_CREATE_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "vectormap_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;


class VectorMapCreateTest: public VectorMapTestBase {

    typedef VectorMapCreateTest                                                 MyType;
    typedef VectorMapTestBase                                                 	Base;

    typedef std::function<void (MyType*, Allocator&, Ctr&)> 					TestFn;

public:

    VectorMapCreateTest(): Base("Create")
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(testOrderedCreation, replayOrderedCreation);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRandomCreation, replayRandomCreation);
    }

    virtual ~VectorMapCreateTest() throw() {}

    void test(TestFn test_fn)
    {
    	DefaultLogHandlerImpl logHandler(out());

    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr map(&allocator);

    	ctr_name_ = map.name();

    	allocator.commit();

    	try {
    		for (iteration_ = 0; iteration_ < size_; iteration_++)
    		{
    			test_fn(this, allocator, map);

    			allocator.commit();
    		}
    	}
    	catch (...) {
    		dump_name_ = Store(allocator);
    		storeTripples(tripples_);
    		throw;
    	}
    }


    void replay(TestFn test_fn)
    {
    	Allocator allocator;
    	DefaultLogHandlerImpl logHandler(out());
    	allocator.getLogger()->setHandler(&logHandler);

    	LoadAllocator(allocator, dump_name_);

    	tripples_ = loadTripples();

    	Ctr ctr(&allocator, CTR_FIND, ctr_name_);

    	test_fn(this, allocator, ctr);

    	check(allocator, "Insert: Container Check Failed", MA_SRC);
    }

    void testOrderedCreation()
    {
    	test(&MyType::orderedCreationTest);
    }

    void replayOrderedCreation()
    {
    	replay(&MyType::orderedCreationTest);
    }

    void testRandomCreation()
    {
    	test(&MyType::randomCreationTest);
    }

    void replayRandomCreation()
    {
    	replay(&MyType::randomCreationTest);
    }

    void orderedCreationTest(Allocator& allocator, Ctr& map)
    {
    	if (!isReplayMode())
    	{
    		data_size_  = getRandom(max_block_size_);
    		data_		= iteration_ & 0xFF;
    	}

    	vector<Value> data = createSimpleBuffer<Value>(data_size_, data_);

    	MemBuffer<Value> buf(data);

    	auto iter = map.create(buf);

    	tripples_.push_back(Tripple(iter.id(), iter.blob_size(), data_));

    	try {
    		checkBlock(iter, iter.id(), data_size_, data_);

    		if (iterator_check_counter_ % iterator_check_count_ == 0)
    		{
    			checkDataFw(tripples_, map);
    			checkDataBw(tripples_, map);
    		}

    		iterator_check_counter_++;
    	}
    	catch(...) {
    		tripples_.pop_back();
    		throw;
    	}
    }

    BigInt getNewRandomId(Ctr& map)
    {
    	BigInt id;

    	do {
    		id = getBIRandom(10000000) + 1;
    	}
    	while(map.contains(id));

    	return id;
    }

    void randomCreationTest(Allocator& allocator, Ctr& map)
    {
    	out()<<iteration_<<endl;

    	if (!isReplayMode())
    	{
    		data_size_  = getRandom(max_block_size_);
    		data_		= iteration_ & 0xFF;
    		key_ 		= getNewRandomId(map);
    	}

    	vector<Value> data = createSimpleBuffer<Value>(data_size_, data_);

    	MemBuffer<Value> buf(data);

    	auto iter = map.create(key_, buf);

    	UInt insertion_pos;
    	for (insertion_pos = 0; insertion_pos < tripples_.size(); insertion_pos++)
    	{
    		if (key_ <= tripples_[insertion_pos].id())
    		{
    			break;
    		}
    	}

    	tripples_.insert(tripples_.begin() + insertion_pos, Tripple(iter.id(), iter.blob_size(), data_));

    	try {
    		checkBlock(iter, iter.id(), data_size_, data_);

    		if (iterator_check_counter_ % iterator_check_count_ == 0)
    		{
    			checkDataFw(tripples_, map);
    			checkDataBw(tripples_, map);
    		}

    		iterator_check_counter_++;
    	}
    	catch(...)
    	{
    		tripples_.erase(tripples_.begin() + insertion_pos);
    		throw;
    	}
    }
};




}



#endif

