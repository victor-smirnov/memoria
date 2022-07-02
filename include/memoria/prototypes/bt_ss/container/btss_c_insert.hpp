
// Copyright 2015-2021 Victor Smirnov
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/prototypes/bt_ss/btss_names.hpp>
#include <memoria/prototypes/bt_ss/btss_batch_input.hpp>

#include <memoria/core/container/macros.hpp>
#include <memoria/api/common/ctr_api_btss.hpp>


#include <memoria/core/tools/object_pool.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss::InsertName)

    using typename Base::TreeNodeConstPtr;
    using typename Base::Position;
    using typename Base::Profile;
    using typename Base::TreePathT;

    using typename Base::CtrSizeT;
    using typename Base::LeafNode;

    using typename Base::BlockIteratorState;
    using typename Base::BlockIteratorStatePtr;

    using CtrInputBuffer = typename TypesType::CtrInputBuffer;

    static constexpr size_t Stream = 0;

    template <typename Entry>
    BlockIteratorStatePtr ctr_insert_entry(
        BlockIteratorStatePtr&& iter,
        const Entry& entry
    )
    {
        auto& self = the_self();
        auto& path = iter->path();

        CtrSizeT idx = iter->iter_leaf_position();

        auto status0 = self.template ctr_try_insert_stream_entry<Stream>(path, idx, entry);

        if (!status0)
        {
            CtrSizeT split_pos = div_2(self.ctr_leaf_sizes(path.leaf())[0]);
            self.ctr_split_leaf(path, Position::create(0, split_pos));

            if (split_pos <= idx) {
                assert_success(self.ctr_get_next_node(path, 0));
                idx -= split_pos;
            }

            iter->iter_set_leaf_position(split_pos);

            auto status1 = self.template ctr_try_insert_stream_entry<Stream>(path, idx, entry);
            if (!status1){
                MEMORIA_MAKE_GENERIC_ERROR("Second insertion attempt failed").do_throw();
            }
        }

        iter->iter_reset_caches();

        self.ctr_update_path(path, 0);

        return std::move(iter);
    }


    template <typename SubstreamPath, typename Entry>
    BlockIteratorStatePtr ctr_update_entry2(
        BlockIteratorStatePtr&& iter,
        const Entry& entry
    )
    {
        auto& self = the_self();
        auto& path = iter->path();

        CtrSizeT idx = iter->iter_leaf_position();
        auto status0 = self.template ctr_try_update_stream_entry<SubstreamPath>(path, idx, entry);

        if (!status0)
        {
            CtrSizeT split_pos = div_2(self.ctr_leaf_sizes(path.leaf())[0]);
            self.ctr_split_leaf(path, Position::create(0, split_pos));

            if (split_pos <= idx) {
                assert_success(self.ctr_get_next_node(path, 0));
                idx -= split_pos;
            }

            iter->iter_set_leaf_position(split_pos);

            auto status1 = self.template ctr_try_update_stream_entry<SubstreamPath>(path, idx, entry);
            if (!status1){
                MEMORIA_MAKE_GENERIC_ERROR("Second insertion attempt failed").do_throw();
            }
        }

        iter->iter_reset_caches();

        self.ctr_update_path(path, 0);

        return std::move(iter);
    }






    BlockIteratorStatePtr ctr_insert_batch(
            BlockIteratorStatePtr&& iter,
            CtrBatchInputFn<CtrInputBuffer> producer)
    {
        auto& self = this->self();

        auto buf = allocate_shared<CtrInputBuffer>(self.store().object_pools());

        btss::BTSSCtrBatchInputProvider<MyType> streaming(self, producer, *buf.get());

        auto pos = Position::create(0, iter->iter_leaf_position());

        self.ctr_insert_provided_data(iter->path(), pos, streaming);

        iter->iter_set_leaf_position(pos[0]);
        iter->iter_reset_caches();

        return std::move(iter);
    }

    BlockIteratorStatePtr ctr_insert_batch(BlockIteratorStatePtr&& iter, CtrInputBuffer& input_buffer)
    {
        auto& self = this->self();

        auto producer = [](CtrInputBuffer&){
            return true;
        };

        btss::BTSSCtrBatchInputProvider<MyType> streaming(self, producer, input_buffer, false);

        auto pos = Position::create(0, iter->iter_leaf_position());

        self.ctr_insert_provided_data(iter->path(), pos, streaming);

        iter->iter_set_leaf_position(pos[0]);
        iter->iter_reset_caches();

        return std::move(iter);
    }



MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
