
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

#include <memoria/prototypes/bt_fl/btfl_tools.hpp>
#include <memoria/prototypes/bt_fl/btfl_batch_input_provider.hpp>

#include <memoria/core/tools/object_pool.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btfl::InsertName)


    using typename Base::LeafNode;
    using typename Base::BlockIteratorState;
    using typename Base::BlockIteratorStatePtr;

    using Base::Streams;

    using typename Base::MyROType;

    using CtrInputBuffer = typename TypesType::CtrInputBuffer;


    BlockIteratorStatePtr ctr_insert_batch(BlockIteratorStatePtr&& iter, CtrBatchInputFn<CtrInputBuffer> provider)
    {
        auto& self = this->self();

        auto buf = allocate_shared<CtrInputBuffer>(self.store().object_pools());

        btfl::CtrBatchInputProvider<MyType> streaming(self, provider, *buf.get());

        auto pos = iter->iter_leafrank();

        self.ctr_insert_provided_data(iter->path(), pos, streaming);

        iter->iter_finish_update(pos);

        return std::move(iter);
    }


    BlockIteratorStatePtr ctr_insert_batch(BlockIteratorStatePtr&& iter, CtrInputBuffer& input_buffer)
    {
        auto& self = this->self();

        auto producer = [](CtrInputBuffer&){
            return true;
        };

        btfl::CtrBatchInputProvider<MyType> streaming(self, &producer, &input_buffer);

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
