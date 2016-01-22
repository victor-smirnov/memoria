
// Copyright Victor Smirnov 2012+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../prototype/btss/btss_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;



template <
    typename CtrName,
	typename AllocatorT 	= SmallInMemAllocator,
	typename ProfileT		= DefaultProfile<>
>
class VectorTest: public BTSSTestBase<CtrName, AllocatorT, ProfileT>
{
    using MyType = VectorTest<CtrName, AllocatorT, ProfileT>;

    using Base = BTSSTestBase<CtrName, AllocatorT, ProfileT>;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Ctr::BranchNodeEntry                                           BranchNodeEntry;

    using Value = typename Ctr::Types::Value;

    using Allocator 	= typename Base::Allocator;
    using AllocatorSPtr = typename Base::AllocatorSPtr;

    BigInt size = 1024*1024;

public:
    VectorTest(StringRef name):
        Base(name)
    {
    	MEMORIA_ADD_TEST(testCreate);
    }

    virtual void createAllocator(AllocatorSPtr& allocator) {
    	this->allocator_ = std::make_shared<Allocator>();
    }


    void testCreate()
    {
    	Ctr ctr(this->allocator_.get(), CTR_CREATE);

    	std::vector<Value> data(size);

    	for (auto& d: data)
    	{
    		d = this->getRandom(100);
    	}

    	ctr.begin().insert(data.begin(), data.size());

    	AssertEQ(MA_SRC, ctr.size(), data.size());

    	std::vector<Value> data2(size);

    	auto read = ctr.begin().read(data2.begin(), data2.size());

    	AssertEQ(MA_SRC, read, data2.size());

    	for (size_t c = 0; c< data.size(); c++)
    	{
    		AssertEQ(MA_SRC, data[c], data2[c]);
    	}
    }
};



}


#endif
