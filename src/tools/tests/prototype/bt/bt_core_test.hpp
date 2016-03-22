// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "bt_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

template <
    typename CtrName,
    typename AllocatorT     = SmallInMemAllocator,
    typename ProfileT       = DefaultProfile<>
>
class BTCoreTest: public BTTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTCoreTest<CtrName, AllocatorT, ProfileT>;

    using Allocator     = typename Base::Allocator;
    using AllocatorSPtr = typename Base::AllocatorSPtr;
    using Ctr           = typename Base::Ctr;

public:

    BTCoreTest(String name): Base(name)
    {

    }

    virtual ~BTCoreTest() throw () {}


    void createAllocator(AllocatorSPtr& allocator) {
        allocator = std::make_shared<Allocator>();
        allocator->mem_limit() = this->hard_memlimit_;
    }

};

}
