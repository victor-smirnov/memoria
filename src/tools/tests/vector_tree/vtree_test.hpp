
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VTREE_TEST_HPP_
#define MEMORIA_TESTS_VTREE_TEST_HPP_

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memory>
#include <map>

namespace memoria {

using namespace std;

class VTreeTest: public SPTestTask {

	typedef SPTestTask														Base;
	typedef VTreeTest 														MyType;

	typedef typename SCtrTF<VTree>::Type                                    Ctr;
	typedef typename Ctr::Iterator                                          Iterator;


	typedef pair<UInt, Int> 												Pair;


	Int remove_check_ 	= 100;

public:

	VTreeTest(StringRef name): SPTestTask(name)
    {
		size_ = 1000;

		MEMORIA_ADD_TEST_PARAM(remove_check_);

		MEMORIA_ADD_TEST(testCreate);
		MEMORIA_ADD_TEST(testRemove);
    }

    virtual ~VTreeTest() throw() {}

    void testCreate()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);

    	allocator.commit();

    	try {

    		auto node = ctr.root_node();


    		allocator.commit();
    		StoreResource(allocator, "vtree");
    	}
    	catch (...) {
    		 Store(allocator);
    		 throw;
    	}
    }

    void testRemove()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);

    	allocator.commit();

    	try {
    		allocator.commit();

    		StoreResource(allocator, "vtree");
    	}
    	catch (...) {
    		Store(allocator);
    		throw;
    	}
    }
};


}


#endif
