
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

#include <memoria/v1/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/v1/prototypes/bt_tl/tools/bttl_tools_random_gen.hpp>
#include <functional>

namespace memoria {
namespace v1 {
namespace bttl {
namespace iobuf {

template <typename CtrT, typename RngT>
class RandomDataInputProvider: public RandomAdapterBase<CtrT, RngT> {

    using Base = RandomAdapterBase<CtrT, RngT>;

    using typename Base::IOBuffer;

    using Base::structure_generator;
    using Base::Streams;


    using Value = typename CtrT::Types::Value;
    using typename Base::CtrSizesT;

public:
    RandomDataInputProvider(const CtrSizesT& structure, const RngT& rng, Int level = 0, size_t iobuffer_size = 65536):
        Base(structure, rng, level, iobuffer_size)
    {}

    virtual Int populate_stream(Int stream, IOBuffer& buffer, Int length)
    {
        if (stream == Streams - 1)
        {
            Int c;
            for (c = 0; c < length; c++)
            {
                auto pos = buffer.pos();
                if (!IOBufferAdapter<Value>::put(buffer, structure_generator().counts()[stream - 1]))
                {
                    buffer.pos(pos);
                    structure_generator().counts()[stream] += c;
                    break;
                }
            }

            structure_generator().counts()[stream] += length;

            return c;
        }
        else {
            structure_generator().counts()[stream] += length;
            return length;
        }
    }
};



template <typename CtrT>
class DeterministicDataInputProvider: public DeterministicAdapterBase<CtrT> {

    using Base = DeterministicAdapterBase<CtrT>;

    using typename Base::IOBuffer;

    using Base::structure_generator;
    using Base::Streams;

    using Key   = typename CtrT::Types::Key;
    using Value = typename CtrT::Types::Value;
    using typename Base::CtrSizesT;

public:
    DeterministicDataInputProvider(const CtrSizesT& structure, Int level = 0, size_t iobuffer_size = 65536):
        Base(structure, level, iobuffer_size)
    {}

    virtual Int populate_stream(Int stream, IOBuffer& buffer, Int length)
    {
        if (stream == Streams - 1)
        {
            Int c;
            for (c = 0; c < length; c++)
            {
                auto pos = buffer.pos();
                if (!IOBufferAdapter<Value>::put(buffer, structure_generator().counts()[stream - 1]))
                {
                    buffer.pos(pos);
                    structure_generator().counts()[stream] += c;
                    break;
                }
            }

            structure_generator().counts()[stream] += length;

            return c;
        }
        else {
            structure_generator().counts()[stream] += length;
            return length;
        }
    }
};


}
}
}}
