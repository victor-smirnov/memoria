
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REPLACE_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REPLACE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>



#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;



template <typename Key, typename Value>
class VectorMapReplaceTest: public VectorMapTestBase<VectorMapReplaceTest<Key, Value>, Key, Value> {

    typedef VectorMapReplaceTest<Key, Value>                                    MyType;
    typedef VectorMapTestBase<VectorMapReplaceTest<Key, Value>, Value, Key>		Base;

protected:

    typedef typename Base::Allocator											Allocator;
    typedef typename Base::Ctr													Ctr;
    typedef typename Base::VMapType												VMapType;
    typedef typename Base::TestFn 												TestFn;


    Int replacements_ 		= 1000;

    Int replacement_pos_;

    Int replacement_type_	= 2;



public:

    VectorMapReplaceTest(): Base("Replace")
    {
    	MEMORIA_ADD_TEST_PARAM(replacements_);

    	MEMORIA_ADD_TEST_PARAM(replacement_type_)->state();
    	MEMORIA_ADD_TEST_PARAM(replacement_pos_)->state();

    	MEMORIA_ADD_TEST_WITH_REPLAY(testRandomReplacement, replayRandomReplacement);

    	MEMORIA_ADD_TEST_WITH_REPLAY(testZeroReplacement0, replayZeroReplacement);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testZeroReplacement1, replayZeroReplacement);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testZeroReplacement2, replayZeroReplacement);
    }

    virtual ~VectorMapReplaceTest() throw() {}


    Int getRandom(Int max)
    {
    	return ::memoria::getRandom(max) + 1;
    }


    void testRandomReplacement()
    {
    	this->testPreFilledMap(&MyType::replacementTest, VMapType::Random, replacements_);
    }

    void replayRandomReplacement()
    {
    	this->replay(&MyType::replacementTest, "RandomReplacement");
    }

    void testZeroReplacement0()
    {
    	replacement_type_ = 0;
    	this->testPreFilledMap(&MyType::replacementTest, VMapType::ZeroData, replacements_);
    }

    void testZeroReplacement1()
    {
    	replacement_type_ = 1;
    	this->testPreFilledMap(&MyType::replacementTest, VMapType::ZeroData, replacements_);
    }

    void testZeroReplacement2()
    {
    	replacement_type_ = 2;
    	this->testPreFilledMap(&MyType::replacementTest, VMapType::ZeroData, replacements_);
    }

    void replayZeroReplacement()
    {
    	this->replay(&MyType::replacementTest, "ZeroReplacement");
    }


    Int getSmallerDataSize(Int current_size)
    {
    	return getRandom(current_size);
    }

    Int getLargerDataSize(Int current_size)
    {
    	Int size;

    	do {
    		size = getRandom(this->max_block_size_);
    	}
    	while (size < current_size);

    	return size;
    }

    void replacementTest(Allocator& allocator, Ctr& map)
    {
    	this->out()<<this->iteration_<<endl;

    	auto& tripples_		= this->tripples_;
    	auto& data_size_	= this->data_size_;
    	auto& data_			= this->data_;
    	auto& key_			= this->key_;

    	if (!this->isReplayMode())
    	{
    		replacement_pos_ 	= ::memoria::getRandom(tripples_.size());

    		auto tripple 		= tripples_[replacement_pos_];

    		key_ 				= tripple.id();
    		data_				= tripple.data();

    		if (replacement_type_ == 0)
    		{
    			data_size_  		= getRandom(this->max_block_size_);
    		}
    		else if (replacement_type_ == 1)
    		{
    			data_size_  		= this->getSmallerDataSize(tripple.size());
    		}
    		else {
    			data_size_  		= this->getLargerDataSize(tripple.size());
    		}
    	}

    	vector<Value> data = createSimpleBuffer<Value>(data_size_, data_);

    	MemBuffer<Value> buf(data);

    	Tripple tripple = tripples_[replacement_pos_];

    	auto iter = map.create(key_, buf);

    	tripples_[replacement_pos_] = Tripple(iter.id(), iter.blob_size(), data_);

    	this->checkMap(map, tripples_, [&]() {
    		tripples_[replacement_pos_] = tripple;
    	});
    }
};


}

#endif

