
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_vector_map_VECTORMAP_CREATE_TEST_HPP_
#define MEMORIA_TESTS_vector_map_VECTORMAP_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "vectormap_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename Key, typename Value>
class VectorMapCreateTest: public VectorMapTestBase<VectorMapCreateTest<Key, Value>, Key, Value> {

	typedef VectorMapTestBase<VectorMapCreateTest<Key, Value>, Key, Value>		Base;

    typedef VectorMapCreateTest<Key, Value>                                     MyType;

protected:

    typedef typename Base::Allocator											Allocator;
    typedef typename Base::Ctr													Ctr;
    typedef typename Base::VMapType												VMapType;
    typedef typename Base::TestFn 												TestFn;


public:

    VectorMapCreateTest(): Base("Create")
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(testOrderedCreation, replayOrderedCreation);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRandomCreation, replayRandomCreation);

        MEMORIA_ADD_TEST(testLongData);
    }

    virtual ~VectorMapCreateTest() throw() {}

    void testLongData()
    {
    	Allocator allocator;
    	Ctr map(&allocator);

    	this->ctr_name_ = map.name();

    	allocator.commit();

    	try {
    		vector<Value> data(1024*1024*10);
    		MemBuffer<Value> buf(data);

    		auto iter = map.create(buf);

    		allocator.commit();

    		//this->StoreAllocator(allocator, this->getResourcePath("alloc.dump"));
    	}
    	catch (...) {
    		this->dump_name_ = this->Store(allocator);
    		throw;
    	}
    }


    void testOrderedCreation()
    {
    	this->testEmptyMap(&MyType::orderedCreationTest);
    }

    void replayOrderedCreation()
    {
    	this->replay(&MyType::orderedCreationTest, "CreateOrdered");
    }

    void testRandomCreation()
    {
    	this->testEmptyMap(&MyType::randomCreationTest);
    }

    void replayRandomCreation()
    {
    	this->replay(&MyType::randomCreationTest, "CreateRandom");
    }

    void orderedCreationTest(Allocator& allocator, Ctr& map)
    {
    	auto& data_size_ = this->data_size_;
    	auto& data_		 = this->data_;
    	auto& tripples_	 = this->tripples_;

    	if (!this->isReplayMode())
    	{
    		data_size_  = getRandom(this->max_block_size_);
    		data_		= this->iteration_ & 0xFF;
    	}

    	vector<Value> data = createSimpleBuffer<Value>(data_size_, data_);

    	MemBuffer<Value> buf(data);

    	auto iter = map.create(buf);

    	tripples_.push_back(Tripple(iter.id(), iter.blob_size(), data_));

    	auto rollback_fn = [&]() {
    		tripples_.pop_back();
    	};

    	this->checkIterator(iter, data_size_, data_, rollback_fn);
    	this->checkMap(map, tripples_, rollback_fn);
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
    	this->out()<<this->iteration_<<endl;

    	auto& data_size_ = this->data_size_;
    	auto& data_		 = this->data_;
    	auto& tripples_	 = this->tripples_;
    	auto& key_		 = this->key_;

    	if (!this->isReplayMode())
    	{
    		data_size_  = getRandom(this->max_block_size_);
    		data_		= this->iteration_ & 0xFF;
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

    	auto rollback_fn = [&]() {
    		tripples_.erase(tripples_.begin() + insertion_pos);
    	};

    	this->checkIterator(iter, data_size_, data_, rollback_fn);
    	this->checkMap(map, tripples_, rollback_fn);
    }
};




}



#endif

