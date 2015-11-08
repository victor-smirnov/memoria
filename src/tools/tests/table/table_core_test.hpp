// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TABLE_CORE_TEST_HPP_
#define MEMORIA_TESTS_TABLE_CORE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../prototype/bttl/bttl_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

template <
    typename CtrName,
	typename AllocatorT 	= SmallInMemAllocator,
	typename ProfileT		= DefaultProfile<>
>
class TableCoreTest: public BTTLTestBase<CtrName, AllocatorT, ProfileT> {

    using Base 	 = BTTLTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = TableCoreTest<CtrName, AllocatorT, ProfileT>;

    using Allocator 	= typename Base::Allocator;
    using AllocatorSPtr = typename Base::AllocatorSPtr;
    using CtrT 			= typename Base::Ctr;

public:

    TableCoreTest(String name): Base(name)
    {
    	MEMORIA_ADD_TEST(basicTest);
    }

    virtual ~TableCoreTest() throw () {}


    virtual void createAllocator(AllocatorSPtr& allocator)
    {
    	allocator = std::make_shared<Allocator>();
    }

    void basicTest() {

    }

};

}

#endif
