
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

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>
#include <memoria/v1/prototypes/bt_fl/io/btfl_io_input_provider_base.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(btfl::InsertName)

public:
    using Types             = typename Base::Types;
    using Iterator          = typename Base::Iterator;

    using LeafNodeT         = typename Types::LeafNode;

public:

#ifdef MMA1_USE_IOBUFFER
    template <typename IOBuffer>
    using CtrInputProviderPool = ObjectPool<btfl::io::IOBufferCtrInputProvider<MyType, IOBuffer>>;
#endif

    static const int32_t Streams = Types::Streams;

    auto insert_iovector(Iterator& iter, io::IOVectorProducer& provider, int64_t start, int64_t length)
    {
        auto& self = this->self();

        std::unique_ptr<io::IOVector> iov = LeafNodeT::template NodeSparseObject<MyType, LeafNodeT>::create_iovector();

        auto id = iter.iter_leaf()->id();

        btfl::io::IOVectorCtrInputProvider<MyType> streaming(self, &provider, iov.get(), start, length);

        auto pos = iter.leafrank(iter.iter_local_pos());

        auto result = self.ctr_insert_provided_data(iter.iter_leaf(), pos, streaming);

        iter.iter_local_pos()  = result.position().sum();
        iter.iter_leaf().assign(result.iter_leaf());

        if (iter.iter_leaf()->id() != id) {
            iter.iter_refresh();
        }

        return streaming.totals();
    }

    struct BTFLIOVectorProducer: io::IOVectorProducer {
        virtual bool populate(io::IOVector& io_vector)
        {
            return true; // provided io_vector has been already populated
        }
    };

    auto insert_iovector(Iterator& iter, io::IOVector& iovector, int64_t start, int64_t length)
    {
        auto& self = this->self();

        std::unique_ptr<io::IOVector> iov = LeafNodeT::template NodeSparseObject<MyType, LeafNodeT>::create_iovector();

        auto id = iter.iter_leaf()->id();

        BTFLIOVectorProducer producer{};

        btfl::io::IOVectorCtrInputProvider<MyType> streaming(self, &producer, &iovector, start, length);

        auto pos = iter.leafrank(iter.iter_local_pos());

        auto result = self.ctr_insert_provided_data(iter.iter_leaf(), pos, streaming);

        iter.iter_local_pos()  = result.position().sum();
        iter.iter_leaf().assign(result.iter_leaf());

        if (iter.iter_leaf()->id() != id) {
            iter.iter_refresh();
        }

        return streaming.totals();
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl::InsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
