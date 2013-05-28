
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_INSERT_DATA_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_INSERT_DATA_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "vectormap_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename Key, typename Value>
class VectorMapInsertDataTest: public VectorMapTestBase<VectorMapInsertDataTest<Key, Value>, Key, Value> {

	typedef VectorMapTestBase<VectorMapInsertDataTest<Key, Value>, Key, Value>	Base;

    typedef VectorMapInsertDataTest<Key, Value>                                 MyType;

protected:

    typedef typename Base::Allocator											Allocator;
    typedef typename Base::Ctr													Ctr;
    typedef typename Base::VMapType												VMapType;
    typedef typename Base::TestFn 												TestFn;

    enum class InsertionType {Start, Middle, End};

    Int insertion_tripple_;
    Int insertion_pos_;

    Int insertions_				= 1000;

    InsertionType type_;



public:

    VectorMapInsertDataTest(): Base("InsertData")
    {
    	MEMORIA_ADD_TEST_PARAM(insertions_);

        MEMORIA_ADD_TEST_PARAM(insertion_tripple_)->state();
        MEMORIA_ADD_TEST_PARAM(insertion_pos_)->state();

    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsertionAtStart, replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsertionAtEnd, replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsertionAtMiddle, replayTest);

    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsertionAtStartZero, replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsertionAtEndZero, replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testInsertionAtMiddleZero, replayTest);
    }

    virtual ~VectorMapInsertDataTest() throw() {}

    void replayTest() {
    	this->replay(&MyType::insertionTest, "InsertData");
    }

    void testInsertionAtStart()
    {
    	type_ = InsertionType::Start;
    	this->testPreFilledMap(&MyType::insertionTest, VMapType::Random, this->insertions_);
    }

    void testInsertionAtEnd()
    {
    	type_ = InsertionType::End;
    	this->testPreFilledMap(&MyType::insertionTest, VMapType::Random, this->insertions_);
    }

    void testInsertionAtMiddle()
    {
    	type_ = InsertionType::Middle;
    	this->testPreFilledMap(&MyType::insertionTest, VMapType::Random, this->insertions_);
    }

    void testInsertionAtStartZero()
    {
    	type_ = InsertionType::Start;
    	this->testPreFilledMap(&MyType::insertionTest, VMapType::ZeroData, this->insertions_);
    }

    void testInsertionAtEndZero()
    {
    	type_ = InsertionType::End;
    	this->testPreFilledMap(&MyType::insertionTest, VMapType::ZeroData, this->insertions_);
    }

    void testInsertionAtMiddleZero()
    {
    	type_ = InsertionType::Middle;
    	this->testPreFilledMap(&MyType::insertionTest, VMapType::ZeroData, this->insertions_);
    }



    void insertionTest(Allocator& allocator, Ctr& map)
    {
    	this->out()<<this->iteration_<<endl;

    	auto& tripples_		= this->tripples_;
    	auto& data_size_	= this->data_size_;
    	auto& data_			= this->data_;
    	auto& key_			= this->key_;

    	if (!this->isReplayMode())
    	{
    		insertion_tripple_ 	= ::memoria::getRandom(tripples_.size());
    		auto tripple 		= tripples_[insertion_tripple_];

    		if (type_ == InsertionType::Start)
    		{
    			insertion_pos_	= 0;
    		}
    		else if (type_ == InsertionType::End)
    		{
    			insertion_pos_	= tripple.size();
    		}
    		else {
    			insertion_pos_ 	= ::memoria::getRandom(tripple.size());
    		}

    		key_ 				= tripple.id();
    		data_				= tripple.data();

    		data_size_  		= getRandom(this->max_block_size_);
    	}

    	vector<Value> data = createSimpleBuffer<Value>(data_size_, data_);

    	MemBuffer<Value> buf(data);

    	Tripple tripple = tripples_[insertion_tripple_];

    	auto iter = map.find(key_);

    	iter.seek(insertion_pos_);
    	iter.insert(buf);

    	tripples_[insertion_tripple_] = Tripple(iter.id(), tripple.size() + data_size_, data_);

    	this->checkMap(map, tripples_, [&]() {
    		tripples_[insertion_tripple_] = tripple;
    	});
    }
};




}



#endif

