
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

#include "../bt/bt_test_base.hpp"

#include <functional>

namespace memoria {
namespace v1 {
namespace tests {

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
    using Ctr           = CtrApi<ContainerTypeName, Profile>;
    using Iterator      = IterApi<ContainerTypeName, Profile>;
    

    using Allocator     = AllocatorType;

    using DataValue     = typename Ctr::DataValue;
    using Entry         = typename Ctr::DataValue;

    using MemBuffer     = std::vector<DataValue>;

    using Base::getRandom;

public:

    BTSSTestBase()
    {}

    MemBuffer createBuffer(int32_t size) {
        return MemBuffer(size);
    }

    virtual MemBuffer createRandomBuffer(int32_t size) = 0;



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

    virtual void fillRandom(Ctr& ctr, int64_t size)
    {
        MemBuffer data = createRandomBuffer(size);

        ctr.end().insert(data);
    }


    virtual void fillRandom(Allocator& alloc, Ctr& ctr, int64_t size)
    {
        int64_t block_size = size > 65536*4 ? 65536*4 : size;

        int64_t total = 0;

        auto iter = ctr.seek(0);

        while (total < size)
        {
            int64_t tmp_size = size - total > block_size ? block_size : size - total;

            MemBuffer data = createRandomBuffer(tmp_size);
            
            ctr.end().insert(data);

            total += tmp_size;
        }
    }


};

}}}
