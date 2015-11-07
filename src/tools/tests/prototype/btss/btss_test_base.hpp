
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTSS_TEST_BASE_HPP_
#define MEMORIA_TESTS_BTSS_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../bt/bt_test_base.hpp"

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

    using Entry 		= typename Ctr::Types::template StreamInputTuple<0>;
    using EntryAdapter 	= typename Ctr::Types::template InputTupleAdapter<0>;

    using MemBuffer		= std::vector<Entry>;


public:

    BTSSTestBase(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();
    }

    virtual ~BTSSTestBase() throw() {}

    MemBuffer createBuffer(Int size) {
    	return MemBuffer(size);
    }

    MemBuffer createRandomBuffer(Int size) {
    	auto buffer = MemBuffer(size);

    	for (auto& v: buffer)
    	{
    		v = EntryAdapter::convert(this->getRandom(100));
    	}

    	return buffer;
    }

    void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
    	AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

    	for (size_t c = 0; c < src.size(); c++)
    	{
    		typename MemBuffer::value_type v1 = src[c];
    		typename MemBuffer::value_type v2 = tgt[c];

    		AssertEQ(source, v1, v2, [=](){return SBuf()<<"c="<<c;});
    	}
    }

    virtual void fillRandom(Ctr& ctr, BigInt size)
    {
        MemBuffer data = this->createRandomBuffer(size);

        btss::IteratorBTSSInputProvider<Ctr, typename MemBuffer::const_iterator> provider(ctr, data.begin(), data.end());

        Iterator iter = ctr.seek(0);
        iter.insert(provider);
    }


    virtual void fillRandom(Allocator& alloc, Ctr& ctr, BigInt size)
    {
        BigInt block_size = size > 65536*4 ? 65536*4 : size;

        BigInt total = 0;

        Iterator iter = ctr.seek(0);

        while (total < size)
        {
            BigInt tmp_size = size - total > block_size ? block_size : size - total;

            MemBuffer data = this->createRandomBuffer(tmp_size);

            iter.insert(data.begin(), data.end());

            alloc.flush();

            total += tmp_size;
        }
    }


};

}


#endif
