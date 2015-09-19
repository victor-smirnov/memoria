
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTSS_TEST_BASE_HPP_
#define MEMORIA_TESTS_BTSS_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include "bt_test_base.hpp"

#include <functional>

namespace memoria {

using namespace std;

template <
	typename ContainerTypeName,
    typename AllocatorType,
	typename Profile
>
class BTSSTestBase: public BTTestBase<ContainerTypeName, AllocatorType, Profile> {

    using MyType = BTSSTestBase<
                ContainerTypeName,
				Profile,
				AllocatorType
    >;

    using Base = BTTestBase<ContainerTypeName, AllocatorType, Profile>;

protected:
    using Ctr 			= typename CtrTF<Profile, ContainerTypeName>::Type;
    using Iterator 		= typename Ctr::Iterator;
    using ID 			= typename Ctr::ID;
    using Accumulator 	= typename Ctr::Accumulator;

    using Allocator 	= AllocatorType;
public:

    BTSSTestBase(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();
    }

    virtual ~BTSSTestBase() throw() {}


};

}


#endif
