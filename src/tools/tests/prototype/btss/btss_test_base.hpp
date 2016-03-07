
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
    using Ctr           = typename CtrTF<Profile, ContainerTypeName>::Type;
    using IteratorPtr   = typename Ctr::IteratorPtr;
    using ID            = typename Ctr::ID;
    using BranchNodeEntry = typename Ctr::BranchNodeEntry;

    using Allocator     = AllocatorType;

    using Entry         = typename Ctr::Types::Entry;

    using MemBuffer     = std::vector<Entry>;

    using Base::getRandom;

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

    virtual MemBuffer createRandomBuffer(Int size) = 0;



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
        MemBuffer data = createRandomBuffer(size);

        auto iter = ctr.end();
        iter->bulk_insert(data.begin(), data.end());
    }


    virtual void fillRandom(Allocator& alloc, Ctr& ctr, BigInt size)
    {
        BigInt block_size = size > 65536*4 ? 65536*4 : size;

        BigInt total = 0;

        auto iter = ctr.seek(0);

        while (total < size)
        {
            BigInt tmp_size = size - total > block_size ? block_size : size - total;

            MemBuffer data = createRandomBuffer(tmp_size);

            iter->bulk_insert(data.begin(), data.end());

            total += tmp_size;
        }
    }


};

}


#endif
