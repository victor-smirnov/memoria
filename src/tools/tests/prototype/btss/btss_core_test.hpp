// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTSS_CORE_TEST_HPP_
#define MEMORIA_TESTS_BTSS_CORE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../../shared/btss_test_base.hpp"
#include "btss_test_factory.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

template <
    typename CtrName,
	typename AllocatorT 	= SmallInMemAllocator,
	typename ProfileT		= SmallProfile<>
>
class BTSSCoreTest: public BTSSTestBase<CtrName, AllocatorT, ProfileT> {

    using Base 	 = BTSSTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTSSCoreTest<CtrName, AllocatorT, ProfileT>;

    using Allocator 	= typename Base::Allocator;
    using AllocatorSPtr = typename Base::AllocatorSPtr;
    using Ctr 			= typename Base::Ctr;

public:

    BTSSCoreTest(String name): Base(name)
    {

    }

    virtual ~BTSSCoreTest() throw () {}


    void createAllocator(AllocatorSPtr& allocator) {
    	allocator = std::make_shared<Allocator>();
    }

};

}

#endif
