
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

protected:
    template <int32_t Stream>
    VoidResult ctr_remove_stream_entry(Iterator& iter, int32_t stream, int32_t idx) noexcept
    {
        auto& self = this->self();

        auto result0 = self.template ctr_try_remove_stream_entry<Stream>(iter, idx);
        MEMORIA_RETURN_IF_ERROR(result0);

        if (!std::get<0>(result0.get()))
        {
            auto split_res = iter.iter_split_leaf(stream, idx);
            MEMORIA_RETURN_IF_ERROR(split_res);

            auto result1 = self.template ctr_try_remove_stream_entry<Stream>(iter, idx);
            MEMORIA_RETURN_IF_ERROR(result1);

            if (!std::get<0>(result1.get()))
            {
                return VoidResult::make_error("Second removal attempt failed");
            }

            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_path(iter.iter_leaf()));
        }
        else {
            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_path(iter.iter_leaf()));

            auto next = self.ctr_get_next_node(iter.iter_leaf());
            MEMORIA_RETURN_IF_ERROR(next);

            if (next.get().isSet())
            {
                auto res = self.ctr_merge_leaf_nodes(iter.iter_leaf(), next.get(), [](const Position&){
                    return VoidResult::of();
                });
                MEMORIA_RETURN_IF_ERROR(res);
            }

            auto prev = self.ctr_get_prev_node(iter.iter_leaf());
            MEMORIA_RETURN_IF_ERROR(prev);

            if (prev.get().isSet())
            {
                auto res = self.ctr_merge_leaf_nodes(prev.get(), iter.iter_leaf(), [&](const Position& sizes){
                    iter.iter_local_pos() += sizes[0];
                    iter.iter_leaf().assign(prev.get());
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
