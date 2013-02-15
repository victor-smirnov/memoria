
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CREATE_CTR_CREATE_CTR_TEST_HPP_
#define MEMORIA_TESTS_CREATE_CTR_CREATE_CTR_TEST_HPP_

#include "../tests_inc.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;






class CreateCtrTest: public SPTestTask {

    typedef CreateCtrTest                                                       MyType;

    typedef KVPair<BigInt, BigInt>                                              Pair;
    typedef vector<Pair>                                                        PairVector;
    typedef SCtrTF<VectorMap<BigInt, Byte>>::Type                               VectorMapCtr;
    typedef SCtrTF<Map1>::Type                                                  MapCtr;
    typedef VectorMapCtr::Iterator                                              VMIterator;

    PairVector pairs_;

    Int map_size_           = 1024*256;
    Int vector_map_size_    = 200;
    Int block_size_         = 1024;

    Int iteration_          = 0;

    BigInt map_name_;
    BigInt vector_map_name_;

public:

    CreateCtrTest(): SPTestTask("CreateCtr")
    {
        MEMORIA_ADD_TEST_PARAM(map_size_)->setDescription("Size of the Map container");
        MEMORIA_ADD_TEST_PARAM(vector_map_size_)->setDescription("Size of the VectorMap container");
        MEMORIA_ADD_TEST_PARAM(block_size_)->setDescription("Size of data block inserted into VectorMap container");

        MEMORIA_ADD_TEST_PARAM(map_name_)->state();
        MEMORIA_ADD_TEST_PARAM(vector_map_name_)->state();
        MEMORIA_ADD_TEST_PARAM(iteration_)->state();


        MEMORIA_ADD_TEST(runCreateCtrTest);
        MEMORIA_ADD_TEST(runStoreTest);
    }

    virtual ~CreateCtrTest() throw() {}

    virtual void setUp()
    {
    	if (btree_random_branching_)
    	{
    		btree_branching_ = 8 + getRandom(100);
    		out()<<"BTree Branching: "<<btree_branching_<<endl;
    	}
    }

    void assertEmpty(const char* src, Allocator& allocator)
    {
    	AssertEQ(src, allocator.size(), 0);
    }

    void assertSize(const char* src, Allocator& allocator, Int size)
    {
    	AssertEQ(src, allocator.size(), size);
    }

    void runCreateCtrTest()
    {
    	DefaultLogHandlerImpl logHandler(out());

    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);
    	allocator.commit();

    	assertEmpty(MA_SRC, allocator);

    	AssertThrows<Exception>(MA_SRC, []{
    		MapCtr map(nullptr);
    	});

    	AssertThrows<Exception>(MA_SRC, [&]{
    		MapCtr map(&allocator, 0);
    	});

    	assertEmpty(MA_SRC, allocator);

    	AssertThrows<NoCtrException>(MA_SRC, [&]{
    		MapCtr map(&allocator, CTR_FIND, 12345);
    	});

    	assertEmpty(MA_SRC, allocator);

    	// Ensure subsequent CTR_FIND with the same name
    	// doesn't affect allocator
    	AssertThrows<NoCtrException>(MA_SRC, [&]{
    		MapCtr map(&allocator, CTR_FIND, 12345);
    	});

    	assertEmpty(MA_SRC, allocator);

    	AssertDoesntThrow(MA_SRC, [&]{
    		MapCtr map(&allocator, CTR_CREATE | CTR_FIND, 12345);
    	});

    	assertSize(MA_SRC, allocator, 1);

    	AssertThrows<CtrTypeException>(MA_SRC, [&]{
    		VectorMapCtr map(&allocator, CTR_FIND, 12345);
    	});

    	assertSize(MA_SRC, allocator, 1);

    	AssertThrows<NoCtrException>(MA_SRC, [&]{
    		VectorMapCtr map(&allocator, CTR_FIND, 12346);
    	});

    	assertSize(MA_SRC, allocator, 1);

    	AssertDoesntThrow(MA_SRC, [&]{
    		VectorMapCtr map(&allocator, CTR_FIND | CTR_CREATE, 12346);
    	});

    	assertSize(MA_SRC, allocator, 2);

    	AssertDoesntThrow(MA_SRC, [&]{
    		VectorMapCtr map(&allocator, CTR_FIND | CTR_CREATE, 12346);
    	});

    	assertSize(MA_SRC, allocator, 2);

    	AssertDoesntThrow(MA_SRC, [&]{
    		VectorMapCtr map(&allocator, CTR_FIND, 12346);
    	});

    	assertSize(MA_SRC, allocator, 2);

    	BigInt name;

    	{	// Container object lifecycle scope.

    		MapCtr map(&allocator);
    		name = map.name();

    		assertSize(MA_SRC, allocator, 3);

    		for (Int c = 0; c < 1000; c++)
    		{
    			map[c] = c + 1;
    		}

    		// Container's data still exists in allocator
    	}	// after control leaves the cope

    	AssertEQ(MA_SRC, name, INITAL_CTR_NAME_COUNTER + 1);

    	assertSize(MA_SRC, allocator, 3);

    	MapCtr map(&allocator, CTR_FIND, name);

    	for (auto& iter: map)
    	{
    		AssertEQ(MA_SRC, iter.key(), iter.value() - 1);
    	}

    	map.drop();

    	assertSize(MA_SRC, allocator, 2);

    	allocator.commit();

    	assertSize(MA_SRC, allocator, 2);


    	BigInt name1 = allocator.createCtrName();

    	allocator.rollback();

    	BigInt name2 = allocator.createCtrName();

    	AssertEQ(MA_SRC, name1, name2);
    }


    void runStoreTest()
    {
        DefaultLogHandlerImpl logHandler(out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        MapCtr map(&allocator);

        map.setBranchingFactor(100);

        map_name_ = map.name();

        BigInt t00 = getTimeInMillis();

        for (Int c = 0; c < map_size_; c++)
        {
            map[getRandom()] = getRandom();
        }

        VectorMapCtr vector_map(&allocator);

        vector_map.setBranchingFactor(100);

        vector_map_name_ = vector_map.name();

        for (Int c = 0; c < vector_map_size_; c++)
        {
            vector_map[getRandom()] = createBuffer<Byte>(getRandom(block_size_), getRandom(256));
        }

        allocator.commit();

        check(allocator, MA_SRC);

        BigInt t0 = getTimeInMillis();

        String name = this->getResourcePath("alloc1.dump");

        StoreAllocator(allocator, name);

        BigInt t1 = getTimeInMillis();

        Allocator new_alloc;

        LoadAllocator(new_alloc, name);

        BigInt t2 = getTimeInMillis();

        out()<<"Store Time: "<<FormatTime(t1 - t0)<<endl;
        out()<<"Load Time:  "<<FormatTime(t2 - t1)<<endl;

        check(new_alloc, MA_SRC);

        MapCtr new_map(&new_alloc, CTR_FIND, map.name());

        AssertEQ(MA_SRC, map.getBranchingFactor(), new_map.getBranchingFactor());

        AssertEQ(MA_SRC, map.getSize(), new_map.getSize());

        auto new_iter = new_map.Begin();

        for (auto iter = map.Begin(); !iter.isEnd(); iter.nextKey(), new_iter.nextKey())
        {
            AssertEQ(MA_SRC, iter.getKey(0), new_iter.getKey(0));
            AssertEQ(MA_SRC, iter.getValue(), new_iter.getValue());
        }

        BigInt t22 = getTimeInMillis();

        VectorMapCtr new_vector_map(&new_alloc, CTR_FIND, vector_map.name());

        AssertEQ(MA_SRC, vector_map.getBranchingFactor(), new_vector_map.getBranchingFactor());

        auto new_vm_iter = new_vector_map.Begin();

        for (auto iter = vector_map.Begin(); iter.isNotEnd(); iter.next(), new_vm_iter.next())
        {
        	AssertEQ(MA_SRC, iter.size(), new_vm_iter.size());

            vector<Byte> data = iter.read();

            checkBufferWritten(new_vm_iter, data, "Array data check failed", MA_SRC);
        }

        BigInt t33 = getTimeInMillis();

        out()<<"Create Time: "<<FormatTime(t0 - t00)<<endl;
        out()<<"check Time:  "<<FormatTime(t22 - t2)<<endl;
        out()<<"check Time:  "<<FormatTime(t33 - t22)<<endl;
    }


};


}


#endif

