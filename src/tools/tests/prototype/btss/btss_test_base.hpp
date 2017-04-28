
// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "../bt/bt_test_base.hpp"
#include "btss_test_tools.hpp"

#include <functional>

namespace memoria {
namespace v1 {

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
    using BranchNodeEntry = typename Ctr::Types::BranchNodeEntry;

    using Allocator     = AllocatorType;

    using Entry         = typename Ctr::Types::Entry;

    using MemBuffer     = std::vector<Entry>;

    using Base::getRandom;

public:

    BTSSTestBase(StringRef name):
        Base(name)
    {}

    virtual ~BTSSTestBase() noexcept {}

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

        BTSSTestInputProvider<Ctr, MemBuffer> provider(data);
        iter->insert_iobuffer(&provider);
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

            BTSSTestInputProvider<Ctr, MemBuffer> provider(data);
            iter->insert_iobuffer(&provider);

            total += tmp_size;
        }
    }


};

}}
