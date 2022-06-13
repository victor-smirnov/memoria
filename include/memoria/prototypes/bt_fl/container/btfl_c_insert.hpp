
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

#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/prototypes/bt_fl/btfl_tools.hpp>
#include <memoria/prototypes/bt_fl/io/btfl_io_input_provider_base.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btfl::InsertName)


    using typename Base::LeafNode;
    using typename Base::BlockIteratorState;
    using typename Base::BlockIteratorStatePtr;

    using Base::Streams;

    using typename Base::MyROType;


    BlockIteratorStatePtr ctr_insert_iovector(BlockIteratorStatePtr&& iter, io::IOVectorProducer& provider, int64_t start, int64_t length)
    {
        auto& self = this->self();

        auto iov_res = LeafNode::template NodeSparseObject<MyType, LeafNode>::create_iovector();

        btfl::io::IOVectorCtrInputProvider<MyType> streaming(self, &provider, iov_res.get(), start, length);

        auto pos = iter->iter_leafrank();

        self.ctr_insert_provided_data(iter->path(), pos, streaming);

        iter->iter_finish_update(pos);

        return std::move(iter);
    }

    struct BTFLIOVectorProducer: io::IOVectorProducer {
        virtual bool populate(io::IOVector&)
        {
            return true; // provided io_vector has been already populated
        }
    };

    BlockIteratorStatePtr ctr_insert_iovector(BlockIteratorStatePtr&& iter, io::IOVector& iovector, int64_t start, int64_t length)
    {
        auto& self = this->self();

        BTFLIOVectorProducer producer{};

        btfl::io::IOVectorCtrInputProvider<MyType> streaming(self, &producer, &iovector, start, length);

        auto pos = iter->iter_leafrank();

        self.ctr_insert_provided_data(iter->path(), pos, streaming);

        iter->iter_finish_update(pos);

        return std::move(iter);
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl::InsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
