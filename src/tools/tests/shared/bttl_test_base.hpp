
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_TEST_BASE_HPP_
#define MEMORIA_TESTS_BTTL_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/prototypes/bt_tl/tools/bttl_random_gen.hpp>

#include "bt_test_base.hpp"

#include <functional>

namespace memoria {

using namespace std;

template <
	typename ContainerTypeName,
    typename AllocatorType,
	typename Profile
>
class BTTLTestBase: public BTTestBase<ContainerTypeName, AllocatorType, Profile> {

    using MyType = BTTLTestBase<
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

    using CtrSizesT		= typename Ctr::Types::Position;
    using CtrSizeT		= typename Ctr::Types::CtrSizeT;

    static const Int Streams = Ctr::Types::Streams;

public:

    BTTLTestBase(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();
    }

    virtual ~BTTLTestBase() throw() {}

    CtrSizesT sampleTreeShape(Int level_limit, Int last_level_limit, CtrSizeT size)
    {
    	CtrSizesT shape;

    	CtrSizesT limits(level_limit);
    	limits[Streams - 1] = last_level_limit;

    	while(shape[0] == 0)
    	{
    		BigInt resource = size;

    		for (Int c = Streams - 1; c > 0; c--)
    		{
    			Int level_size = getRandom(limits[c]) + 1;

    			shape[c] = level_size;

    			resource = resource / level_size;
    		}

    		shape[0] = resource;
    	}

    	return shape;
    }

    template <typename Provider>
    void fillCtr(Ctr& ctr, Provider& provider)
    {
    	auto iter = ctr.seek(0);

    	long t0 = getTimeInMillis();

    	ctr.insertData(iter.leaf(), CtrSizesT(), provider);

    	long t1 = getTimeInMillis();

    	this->out()<<"Creation time: "<<FormatTime(t1 - t0)<<endl;

    	auto sizes = ctr.sizes();

//    	AssertEQ(MA_SRC, provider.consumed(), sizes);

    	auto ctr_totals = ctr.total_counts();

    	this->out()<<"Totals: "<<ctr_totals<<" "<<sizes<<endl;

    	AssertEQ(MA_SRC, ctr_totals, sizes);

    	this->allocator()->commit();

    	this->storeAllocator("core.dump");
    }

};

}


#endif
