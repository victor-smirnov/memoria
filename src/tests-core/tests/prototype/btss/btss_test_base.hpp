
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
namespace tests {

template <
    typename ContainerTypeName,
    typename Profile,
    typename StoreType
>
class BTSSTestBase: public BTTestBase<ContainerTypeName, Profile, StoreType> {

    using MyType = BTSSTestBase;

    using Base = BTTestBase<ContainerTypeName, Profile, StoreType>;

protected:
    using CtrApi    = ICtrApi<ContainerTypeName, Profile>;
    using StorePtr  = StoreType;

    using MemBuffer = typename CtrApi::BufferT;

    using Base::getRandom;

public:

    BTSSTestBase()
    {}


    virtual CtrSharedPtr<MemBuffer> createRandomBuffer(int32_t size) = 0;

    void compareBuffers(const MemBuffer& expected, const MemBuffer& actual, const char* source)
    {
        assert_equals(expected.size(), actual.size(), "buffer sizes are not equal", "");

        for (size_t c = 0; c < expected.size(); c++)
        {
            auto v1 = expected[c];
            auto v2 = actual[c];

            assert_equals(v1, v2, "position = {}", c);
        }
    }

    virtual void fillRandom(CtrApi& ctr, int64_t size)
    {
        auto data = createRandomBuffer(size);
        auto ctr_size = ctr.size().get_or_throw();
        ctr.insert(ctr_size, *data).get_or_throw();
    }


    virtual void fillRandom(CtrApi& ctr, int64_t size, int64_t block_size)
    {

        if (block_size > size) {
            block_size = size;
        }

        int64_t total = 0;

        while (total < size)
        {
            int64_t tmp_size = size - total > block_size ? block_size : size - total;
            fillRandom(ctr, tmp_size);
            total += tmp_size;
        }
    }


};

}}
