
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

    typedef VectorMapReplaceTest                                                 MyType;
    typedef VectorMapTestBase													Base;

protected:

    typedef std::function<void (MyType*, Allocator&, Ctr&)> 					TestFn;

public:

    VectorMapReplaceTest(): Base("Replace")
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(testRandomReplacement, replayRandomReplacement);
    }

    virtual ~VectorMapReplaceTest() throw() {}

    VMapData createRandomVMap(Ctr& map, Int size)
    {
    	VMapData tripples;

    	for (Int c = 0; c < size; c++)
    	{
    		Int	data_size 	= getRandom(max_block_size_);
    		Int	data		= c & 0xFF;
    		Int	key 		= getNewRandomId(map);

    		vector<Value> vdata = createSimpleBuffer<Value>(data_size, data);

    		MemBuffer<Value> buf(vdata);

    		auto iter = map.create(key, buf);

    		UInt insertion_pos;
    		for (insertion_pos = 0; insertion_pos < tripples.size(); insertion_pos++)
    		{
    			if (key <= tripples[insertion_pos].id())
    			{
    				break;
    			}
    		}

    		tripples.insert(tripples.begin() + insertion_pos, Tripple(iter.id(), iter.blob_size(), data));
    	}

    	return tripples;
    }


    void test(TestFn test_fn)
    {
    	DefaultLogHandlerImpl logHandler(out());

    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr map(&allocator);

    	ctr_name_ = map.name();

    	tripples_ = createRandomVMap(map, size_);

    	checkDataFw(tripples_, map);

    	allocator.commit();

    	StoreAllocator(allocator, getResourcePath("arc-alloc.dump"));

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

    	check(allocator, "Replace: Container Check Failed", MA_SRC);
    }


    void testRandomReplacement()
    {
    	test(&MyType::randomReplacementTest);
    }

    void replayRandomReplacement()
    {
    	replay(&MyType::randomReplacementTest);
    }


    void randomReplacementTest(Allocator& allocator, Ctr& map)
    {
    	if (!isReplayMode())
    	{
    		key_ = tripples_[getRandom(tripples_.size())].id();
    	}


    	bool removed = map.remove(key_);

    	AssertTrue(MA_SRC, removed);

    	Int insertion_pos = -1;
    	for (UInt idx = 0; idx < tripples_.size(); idx++)
    	{
    		if (key_ == tripples_[idx].id())
    		{
    			insertion_pos = idx;
    			break;
    		}
    	}

    	AssertGE(MA_SRC, insertion_pos, 0);

    	auto tripple = tripples_[insertion_pos];

    	tripples_.erase(tripples_.begin() + insertion_pos);

    	try {
    		if (iterator_check_counter_ % iterator_check_count_ == 0)
    		{
    			checkDataFw(tripples_, map);
    			checkDataBw(tripples_, map);
    		}

    		iterator_check_counter_++;
    	}
    	catch(...)
    	{
    		tripples_.insert(tripples_.begin() + insertion_pos, tripple);
    		throw;
    	}
    }
};


}

#endif

