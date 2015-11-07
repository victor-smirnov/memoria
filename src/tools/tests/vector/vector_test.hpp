
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_vctr_vctr_TEST_HPP_
#define MEMORIA_TESTS_vctr_vctr_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../shared/inmem_sequence_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;



template <
    typename CtrName,
	typename AllocatorT 	= SmallInMemAllocator,
	typename ProfileT		= SmallProfile<>
>
class VectorTest: public BTSSTestBase<CtrName, AllocatorT, ProfileT>
{
    using MyType = VectorTest<CtrName, AllocatorT, ProfileT>;

    using Base = BTSSTestBase<CtrName, AllocatorT, ProfileT>;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Base::ID                                                   ID;

public:
    VectorTest(StringRef name):
        Base(name)
    {

    }

};



}


#endif
