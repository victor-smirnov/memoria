
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REMOVE_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REMOVE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>



#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;




class VectorMapRemoveTest: public VectorMapTestBase {

    typedef VectorMapRemoveTest                                                 MyType;
    typedef VectorMapTestBase													Base;

protected:

    typedef std::function<void (MyType*, Allocator&, Ctr&)> 					TestFn;

public:

    VectorMapRemoveTest(): Base("Remove")
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(testRandomRemoval, replayRandomRemoval);
    }

    virtual ~VectorMapRemoveTest() throw() {}

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

    	check(allocator, "Remove: Container Check Failed", MA_SRC);
    }


    void testRandomRemoval()
    {
    	test(&MyType::randomRemovalTest);
    }

    void replayRandomRemoval()
    {
    	replay(&MyType::randomRemovalTest);
    }


    void randomRemovalTest(Allocator& allocator, Ctr& map)
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

