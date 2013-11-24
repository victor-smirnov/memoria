
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_DBLMAP2_TEST_HPP_
#define MEMORIA_TESTS_DBLMAP2_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>

#include <memoria/containers/dbl_map/dblmap_factory.hpp>


namespace memoria {

using namespace memoria::vapi;
using namespace std;


class DblMap2Test: public SPTestTask {

    typedef SPTestTask      													Base;

    typedef DblMap2Test                                     					MyType;

protected:

    typedef typename Base::Allocator                                            Allocator;
    typedef typename SCtrTF<DblMrkMap2<BigInt, BigInt, 2>>::Type				Ctr;
    typedef typename Ctr::Iterator												Iterator;

    typedef typename Ctr::Types::Key 											Key;
    typedef typename Ctr::Types::Value											Value;


public:

    DblMap2Test(StringRef name): Base(name)
    {
    	this->size_ = 1024;

    	Ctr::initMetadata();

    	MEMORIA_ADD_TEST(testIter);
    	MEMORIA_ADD_TEST(testCreateRemove);

//    	MEMORIA_ADD_TEST(testFind);
    }

    virtual ~DblMap2Test() throw() {}


    void testIter() {
    	Allocator allocator;

    	Ctr ctr(&allocator, CTR_CREATE);

    	auto iter1 = ctr.Begin();
    	auto iter2 = ctr.End();

    	auto iter3 = ctr.RBegin();
    	auto iter4 = ctr.REnd();
    }

    void testCreateRemove()
    {
    	Allocator allocator;

    	Ctr ctr(&allocator, CTR_CREATE);


    	for (int c = 1; c <= 3; c++)
    	{
    		auto iter = ctr.create(c);

    		int max = c == 2 ? 2000 : 20;

    		for (int d = 1; d < max; d++)
    		{
    			iter.insert(d, d);
    		}
    	}

    	auto iter = ctr.find(2);

    	iter.skipFw(5);
    	iter.removeRange(1990);

    	iter.dump();
    }

    void testFind()
    {
    	Allocator allocator;

    	Ctr ctr(&allocator, CTR_CREATE);

    	for (int c = 0; c < 100; c++)
    	{
//    		ctr.insert({getRandom(1000), c + 1});
    	}

    	ctr.Begin().dump();
    }

};




}



#endif

