
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REMOVE_DATA_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REMOVE_DATA_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "vectormap_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename Key, typename Value>
class VectorMapRemoveDataTest: public VectorMapTestBase<VectorMapRemoveDataTest<Key, Value>, Key, Value> {

	typedef VectorMapTestBase<VectorMapRemoveDataTest<Key, Value>, Key, Value>	Base;

    typedef VectorMapRemoveDataTest<Key, Value>                                 MyType;

protected:

    typedef typename Base::Allocator											Allocator;
    typedef typename Base::Ctr													Ctr;
    typedef typename Base::VMapType												VMapType;
    typedef typename Base::TestFn 												TestFn;

    enum class RemovalType {Start, Middle, End};

    Int target_tripple_;
    Int target_pos_;

    Int removes_				= 1000;

    RemovalType type_;



public:

    VectorMapRemoveDataTest(): Base("RemoveData")
    {
    	MEMORIA_ADD_TEST_PARAM(removes_);

        MEMORIA_ADD_TEST_PARAM(target_tripple_)->state();
        MEMORIA_ADD_TEST_PARAM(target_pos_)->state();

    	MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtStart, 	replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtEnd, 		replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtMiddle, 	replayTest);

    	MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtStartZero, replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtEndZero, 	replayTest);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtMiddleZero, replayTest);
    }

    virtual ~VectorMapRemoveDataTest() throw() {}

    void replayTest() {
    	this->replay(&MyType::removalTest, "RemoveData");
    }

    void testRemoveAtStart()
    {
    	type_ = RemovalType::Start;
    	this->testPreFilledMap(&MyType::removalTest, VMapType::Random, this->removes_);
    }

    void testRemoveAtEnd()
    {
    	type_ = RemovalType::End;
    	this->testPreFilledMap(&MyType::removalTest, VMapType::Random, this->removes_);
    }

    void testRemoveAtMiddle()
    {
    	type_ = RemovalType::Middle;
    	this->testPreFilledMap(&MyType::removalTest, VMapType::Random, this->removes_);
    }

    void testRemoveAtStartZero()
    {
    	type_ = RemovalType::Start;
    	this->testPreFilledMap(&MyType::removalTest, VMapType::ZeroData, this->removes_);
    }

    void testRemoveAtEndZero()
    {
    	type_ = RemovalType::End;
    	this->testPreFilledMap(&MyType::removalTest, VMapType::ZeroData, this->removes_);
    }

    void testRemoveAtMiddleZero()
    {
    	type_ = RemovalType::Middle;
    	this->testPreFilledMap(&MyType::removalTest, VMapType::ZeroData, this->removes_);
    }



    void removalTest(Allocator& allocator, Ctr& map)
    {
    	this->out()<<this->iteration_<<endl;

    	auto& tripples_		= this->tripples_;
    	auto& data_size_	= this->data_size_;
    	auto& data_			= this->data_;
    	auto& key_			= this->key_;

    	if (!this->isReplayMode())
    	{
    		target_tripple_ 	= ::memoria::getRandom(tripples_.size());
    		auto tripple 		= tripples_[target_tripple_];

    		if (type_ == RemovalType::Start)
    		{
    			target_pos_	= 0;
    			data_size_	= ::memoria::getRandom(tripple.size() + 1);
    		}
    		else if (type_ == RemovalType::End)
    		{
    			data_size_	= ::memoria::getRandom(tripple.size() + 1);
    			target_pos_	= tripple.size() - data_size_;
    		}
    		else {
    			data_size_	= ::memoria::getRandom(tripple.size());
    			target_pos_ = ::memoria::getRandom(tripple.size() - data_size_);
    		}

    		key_ 				= tripple.id();
    		data_				= tripple.data();
    	}

    	Tripple tripple = tripples_[target_tripple_];

    	auto iter = map.find(key_);

    	iter.seek(target_pos_);

    	iter.remove(data_size_);

    	tripples_[target_tripple_] = Tripple(iter.id(), tripple.size() - data_size_, data_);

    	this->checkMap(map, tripples_, [&]() {
    		tripples_[target_tripple_] = tripple;
    	});
    }
};




}



#endif

