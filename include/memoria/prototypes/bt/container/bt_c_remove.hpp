
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


#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::RemoveName)

    typedef typename Base::Types                                                Types;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    using typename Base::TreePathT;

protected:
    template <int32_t Stream>
    VoidResult ctr_remove_stream_entry(Iterator& iter, int32_t stream, int32_t idx) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(remove_entry_result0, self.template ctr_try_remove_stream_entry<Stream>(iter, idx));
        if (!std::get<0>(remove_entry_result0))
        {
            // FIXME: split at the middle of the leaf!
            MEMORIA_TRY_VOID(iter.iter_split_leaf(stream, idx));
            MEMORIA_TRY(remove_entry_result1, self.template ctr_try_remove_stream_entry<Stream>(iter, idx));

            if (!std::get<0>(remove_entry_result1))
            {
                return VoidResult::make_error("Second removal attempt failed");
            }

            MEMORIA_TRY_VOID(self.ctr_update_path(iter.path(), 0));
        }
        else {
            MEMORIA_TRY_VOID(self.ctr_update_path(iter.path(), 0));

            MEMORIA_TRY_VOID(self.ctr_check_path(iter.path(), 0));

            TreePathT next_path = iter.path();
            MEMORIA_TRY(has_next, self.ctr_get_next_node(next_path, 0));

            if (has_next)
            {
                auto res = self.ctr_merge_leaf_nodes(iter.path(), next_path, [](const Position&){
                    return VoidResult::of();
                });
                MEMORIA_RETURN_IF_ERROR(res);
            }

            TreePathT prev_path = iter.path();

            MEMORIA_TRY(has_prev, self.ctr_get_prev_node(prev_path, 0));

            if (has_prev)
            {
                auto res = self.ctr_merge_leaf_nodes(prev_path, iter.path(), false, [&](const Position& sizes){
                    iter.iter_local_pos() += sizes[0];
                    iter.path() = prev_path;
                    return VoidResult::of();
                });
                MEMORIA_RETURN_IF_ERROR(res);
            }
        }

        return VoidResult::of();
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
