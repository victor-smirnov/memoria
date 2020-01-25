
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

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    template <int32_t Stream, typename Entry>
    Result<SplitStatus> ctr_insert_stream_entry(Iterator& iter, int32_t stream, int32_t idx, const Entry& entry) noexcept
    {
        using ResultT = Result<SplitStatus>;

        auto& self = this->self();

        auto result0 = self.template ctr_try_insert_stream_entry<Stream>(iter, idx, entry);
        MEMORIA_RETURN_IF_ERROR(result0);

        SplitStatus split_status;

        if (!std::get<0>(result0.get()))
        {
            auto split_result = iter.iter_split_leaf(stream, idx);
            MEMORIA_RETURN_IF_ERROR(split_result);

            split_status = split_result.get().type();

            auto result1 = self.template ctr_try_insert_stream_entry<Stream>(iter, split_result.get().stream_idx(), entry);
            MEMORIA_RETURN_IF_ERROR(result1);

            if (!std::get<0>(result1.get()))
            {
                return ResultT::make_error("Second insertion attempt failed");
            }
        }
        else {
            split_status = SplitStatus::NONE;
        }

        MEMORIA_TRY_VOID(self.ctr_update_path(iter.path(), 0));

        return ResultT::of(split_status);
    }




    template <int32_t Stream>
    Result<SplitStatus> ctr_insert_stream_entry0(Iterator& iter, int32_t structure_idx, int32_t stream_idx, std::function<Result<OpStatus> (int, int)> insert_fn) noexcept
    {
        using ResultT = Result<SplitStatus>;
        auto& self = this->self();

        auto result0 = self.ctr_with_block_manager(iter.iter_leaf(), structure_idx, stream_idx, insert_fn);
        MEMORIA_RETURN_IF_ERROR(result0);

        SplitStatus split_status;

        if (!result0.get())
        {
            auto split_result = iter.split(Stream, stream_idx);
            MEMORIA_RETURN_IF_ERROR(split_result);

            split_status = split_result.type();

            auto result1 = self.ctr_with_block_manager(iter.iter_leaf(), iter.iter_local_pos(), split_result.stream_idx(), insert_fn);
            MEMORIA_RETURN_IF_ERROR(result1);

            if (!result1.get())
            {
                return ResultT::make_error("Second insertion attempt failed");
            }
        }
        else {
            split_status = SplitStatus::NONE;
        }

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_path(iter.iter_leaf()));

        return ResultT::of(split_status);
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
