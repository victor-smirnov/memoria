
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




class VectorMapReplaceTest: public VectorMapTestBase {

    typedef VectorMapReplaceTest                                                MyType;
    typedef VectorMapTestBase													Base;

protected:

    typedef std::function<void (MyType*, Allocator&, Ctr&)> 					TestFn;

    Int replacements_ 		= 1000;

    Int replacement_pos_;

    Int replacement_type_	= 2;

    enum class VMapType {Random, ZeroData};

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




    void test(TestFn test_fn, VMapType map_type)
    {
    	DefaultLogHandlerImpl logHandler(out());

    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr map(&allocator);

    	ctr_name_ = map.name();

    	tripples_ = map_type == VMapType::Random ? createRandomVMap(map, size_) : createZeroDataVMap(map, size_);

    	checkDataFw(tripples_, map);

    	allocator.commit();

    	try {
    		for (iteration_ = 0; iteration_ < replacements_; iteration_++)
    		{
    			test_fn(this, allocator, map);

    			allocator.commit();
    		}
    	}
    	catch (Exception ex) {
    		cout<<ex<<endl;
    		dump_name_ = Store(allocator);
    		storeTripples(tripples_);
    		throw;
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

    	try {
    		check(allocator, "Replace: Container Check Failed", MA_SRC);
    	}
    	catch (...) {
    		allocator.commit();
    		StoreAllocator(allocator, "replay-invalid.dump");
    		throw;
    	}
    }

    Int getRandom(Int max)
    {
    	return ::memoria::getRandom(max) + 1;
    }


    void testRandomReplacement()
    {
    	test(&MyType::replacementTest, VMapType::Random);
    }

    void replayRandomReplacement()
    {
    	replay(&MyType::replacementTest);
    }

    void testZeroReplacement0()
    {
    	replacement_type_ = 0;
    	test(&MyType::replacementTest, VMapType::ZeroData);
    }

    void testZeroReplacement1()
    {
    	replacement_type_ = 1;
    	test(&MyType::replacementTest, VMapType::ZeroData);
    }

    void testZeroReplacement2()
    {
    	replacement_type_ = 2;
    	test(&MyType::replacementTest, VMapType::ZeroData);
    }

    void replayZeroReplacement()
    {
    	replay(&MyType::replacementTest);
    }


    Int getSmallerDataSize(Int current_size)
    {
    	return getRandom(current_size);
    }

    Int getLargerDataSize(Int current_size)
    {
    	Int size;

    	do {
    		size = getRandom(max_block_size_);
    	}
    	while (size < current_size);

    	return size;
    }

    void replacementTest(Allocator& allocator, Ctr& map)
    {
    	out()<<iteration_<<endl;

    	if (!isReplayMode())
    	{
    		replacement_pos_ 	= ::memoria::getRandom(tripples_.size());

    		auto tripple 		= tripples_[replacement_pos_];

    		key_ 				= tripple.id();
    		data_				= tripple.data();

    		if (replacement_type_ == 0)
    		{
    			data_size_  		= getRandom(max_block_size_);
    		}
    		else if (replacement_type_ == 1)
    		{
    			data_size_  		= getSmallerDataSize(tripple.size());
    		}
    		else {
    			data_size_  		= getLargerDataSize(tripple.size());
    		}
    	}

    	vector<Value> data = createSimpleBuffer<Value>(data_size_, data_);

    	MemBuffer<Value> buf(data);

    	Tripple tripple = tripples_[replacement_pos_];

    	auto iter = map.create(key_, buf);

    	tripples_[replacement_pos_] = Tripple(iter.id(), iter.blob_size(), data_);

    	try {
    		if (iterator_check_counter_ % iterator_check_count_ == 0)
    		{
    			checkDataFw(tripples_, map);
    			checkDataBw(tripples_, map);
    		}

    		iterator_check_counter_++;

//    		if (!isReplayMode()) {
//    			allocator.commit();
//    			StoreAllocator(allocator, getResourcePath((SBuf()<<"alloc-"<<iteration_<<".dump").str()));
//    		}

    	}
    	catch(...)
    	{
    		tripples_[replacement_pos_] = tripple;
    		throw;
    	}
    }
};


}

#endif

