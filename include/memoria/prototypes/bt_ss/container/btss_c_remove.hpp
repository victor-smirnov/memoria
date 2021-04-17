
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


#include <memoria/prototypes/bt_ss/btss_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss::RemoveName)

    using typename Base::NodeBasePtr;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;




    VoidResult ctr_remove_entry(Iterator& iter, bool update_iterator = true) noexcept
    {
        auto& self = this->self();

        int32_t idx = iter.iter_local_pos();
        MEMORIA_TRY(remove_entry_result, self.ctr_remove_entry(iter.path(), idx));

        if (update_iterator)
        {
            iter.refresh_iovector_view();
            if (MMA_UNLIKELY(remove_entry_result.leaf_changed))
            {
                iter.iter_local_pos() = remove_entry_result.new_idx;
                MEMORIA_TRY_VOID(iter.iter_refresh());
            }
        }

        return VoidResult::of();
    }


    struct RemoveEntryResult {
        bool leaf_changed;
        int32_t new_idx;
    };

    Result<RemoveEntryResult> ctr_remove_entry(TreePathT& path, int32_t idx) noexcept
    {
        using ResultT = Result<RemoveEntryResult>;
        auto& self = this->self();

        MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));

        RemoveEntryResult result{false, idx};

        MEMORIA_TRY(removed, self.template ctr_try_remove_stream_entry<0>(path, idx));
        if (!removed)
        {
            MEMORIA_TRY(split_result, self.split_leaf_in_a_half(path, idx));

            if (split_result.type() == SplitStatus::RIGHT)
            {
                result.leaf_changed = true;
                result.new_idx = idx = split_result.stream_idx();
            }

            MEMORIA_TRY(removed_try2, self.template ctr_try_remove_stream_entry<0>(path, idx));

            if (!removed_try2)
            {
                return MEMORIA_MAKE_GENERIC_ERROR("BTSS error: second entry removal attempt failed");
            }
        }

        MEMORIA_TRY_VOID(self.ctr_update_path(path, 0));
        MEMORIA_TRY_VOID(self.ctr_check_path(path, 0));

        TreePathT next_path = path;
        MEMORIA_TRY(has_next, self.ctr_get_next_node(next_path, 0));
        if (has_next)
        {
            MEMORIA_TRY_VOID(self.ctr_merge_leaf_nodes(path, next_path));
        }
        else {
            TreePathT prev_path = path;

            MEMORIA_TRY(has_prev, self.ctr_get_prev_node(prev_path, 0));

            if (has_prev)
            {
                MEMORIA_TRY(left_sizes, self.ctr_get_leaf_sizes(prev_path.leaf()));
                MEMORIA_TRY(left_merge_result, self.ctr_merge_leaf_nodes(prev_path, path, false));
                if (left_merge_result)
                {
                    path = prev_path;

                    result.leaf_changed = true;
                    result.new_idx += left_sizes[0];
                }
            }
        }

        return ResultT::of(result);
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
