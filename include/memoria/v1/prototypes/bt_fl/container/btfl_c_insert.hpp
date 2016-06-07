
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>
#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::btfl::InsertName)

public:
    using Types             = typename Base::Types;
    using Iterator          = typename Base::Iterator;

protected:

    template <typename IOBuffer>
    using CtrInputProviderPool = ObjectPool<btfl::io::IOBufferCtrInputProvider<MyType, IOBuffer>>;


    static const Int Streams = Types::Streams;

    template <typename IOBuffer>
    auto bulkio_insert(Iterator& iter, BufferProducer<IOBuffer>& provider, const Int initial_capacity = 4000)
    {
        auto& self = this->self();

        auto id = iter.leaf()->id();

        auto streamingProvider  = self.pools().get_instance(PoolT<CtrInputProviderPool<IOBuffer>>()).get_unique(self, initial_capacity);
        auto iobuffer 			= self.pools().get_instance(PoolT<ObjectPool<IOBuffer>>()).get_unique(65536);

        streamingProvider->init(&provider, iobuffer.get());

        auto pos = iter.leafrank(iter.idx());

        auto result = self.insert_provided_data(iter.leaf(), pos, *streamingProvider.get());

        iter.idx()  = result.position().sum();
        iter.leaf() = result.leaf();

        if (iter.leaf()->id() != id) {
            iter.refresh();
        }

        return streamingProvider->totals();
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::btfl::InsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
