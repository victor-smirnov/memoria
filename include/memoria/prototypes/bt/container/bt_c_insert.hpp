
// Copyright 2011 Victor Smirnov
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
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::InsertName)

    using typename Base::TreeNodePtr;
    using typename Base::Iterator;

    template <int32_t Stream, typename Entry>
    SplitStatus ctr_insert_stream_entry(Iterator& iter, int32_t stream, int32_t idx, const Entry& entry)
    {
        auto& self = the_self();

        auto& path = iter.path();
        self.ctr_check_path(path);

        auto status0 = self.template ctr_try_insert_stream_entry<Stream>(iter, idx, entry);

        SplitStatus split_status;

        if (!status0)
        {
            auto split_result = iter.iter_split_leaf(stream, idx);

            split_status = split_result.type();

            auto status1 = self.template ctr_try_insert_stream_entry<Stream>(iter, split_result.stream_idx(), entry);
            if (!status1)
            {
                MEMORIA_MAKE_GENERIC_ERROR("Second insertion attempt failed").do_throw();
            }
        }
        else {
            split_status = SplitStatus::NONE;
        }

        self.ctr_update_path(iter.path(), 0);

        return split_status;
    }




    template <int32_t Stream>
    SplitStatus ctr_insert_stream_entry0(Iterator& iter, int32_t structure_idx, int32_t stream_idx, std::function<VoidResult (int, int)> insert_fn)
    {
        auto& self = this->self();

        auto result0 = self.ctr_with_block_manager(iter.iter_leaf(), structure_idx, stream_idx, insert_fn);

        SplitStatus split_status;

        if (!result0)
        {
            auto split_result = iter.split(Stream, stream_idx);
            split_status = split_result.type();

            auto result1 = self.ctr_with_block_manager(iter.iter_leaf(), iter.iter_local_pos(), split_result.stream_idx(), insert_fn);
            if (!result1)
            {
                MEMORIA_MAKE_GENERIC_ERROR("Second insertion attempt failed").do_throw();
            }
        }
        else {
            split_status = SplitStatus::NONE;
        }

        self.ctr_update_path(iter.iter_leaf());

        return split_status;
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
