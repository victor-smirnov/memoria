
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

    using typename Base::TreeNodePtr;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::CtrSizeT;

    using typename Base::BlockIteratorStatePtr;
    using typename Base::BlockIteratorState;


    void ctr_remove(CtrSizeT from, CtrSizeT to)
    {
        auto& self = this->self();

        auto ii_from = self.ctr_seek_entry(from);
        auto ii_to = self.ctr_seek_entry(to);

        Position p_from = Position::create(0, ii_from->iter_leaf_position());
        Position p_to   = Position::create(0, ii_to->iter_leaf_position());

        self.ctr_remove_entries(
            ii_from->path(), p_from,
            ii_to->path(), p_to
        );
    }

    void ctr_remove_from(CtrSizeT from)
    {
        auto& self = this->self();

        auto ii_from = self.ctr_seek_entry(from);
        Position p_from = Position::create(0, ii_from->iter_leaf_position());

        self.ctr_remove_nodes_at_end(
            ii_from->path(), p_from
        );
    }

    void ctr_remove_up_to(CtrSizeT pos)
    {
        auto& self = this->self();

        auto ii_to = self.ctr_seek_entry(pos);
        Position p_to = Position::create(0, ii_to->iter_leaf_position());

        self.ctr_remove_nodes_from_start(
            ii_to->path(), p_to
        );
    }



    void ctr_remove_entry(Iterator& iter, bool update_iterator = true)
    {
        auto& self = this->self();

        int32_t idx = iter.iter_local_pos();
        auto remove_entry_result = self.ctr_remove_entry(iter.path(), idx);

        if (update_iterator)
        {
            iter.refresh_iovector_view();
            if (MMA_UNLIKELY(remove_entry_result.leaf_changed))
            {
                iter.iter_local_pos() = remove_entry_result.new_idx;
                iter.iter_refresh();
            }
        }
    }

    BlockIteratorStatePtr ctr_remove_entry2(BlockIteratorStatePtr&& iter)
    {
        auto& self = this->self();

        auto idx = iter->iter_leaf_position();
        auto remove_entry_result = self.ctr_remove_entry(iter->path(), idx);

        CtrSizeT leaf_size = self.ctr_leaf_sizes(iter->path().leaf())[0];
        iter->finish_ride(remove_entry_result.new_idx, leaf_size);

        if (remove_entry_result.new_idx == leaf_size) {
            auto next_chunk = iter->next_chunk();
            if (next_chunk) {
                iter = memoria_static_pointer_cast<BlockIteratorState>(next_chunk);
            }
        }

        return std::move(iter);
    }


    struct RemoveEntryResult {
        bool leaf_changed;
        CtrSizeT new_idx;
    };

    RemoveEntryResult ctr_remove_entry(TreePathT& path, CtrSizeT idx)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, 0);

        RemoveEntryResult result{false, idx};

        PkdUpdateStatus status = self.template ctr_try_remove_stream_entry<0>(path, idx);
        if (!is_success(status))
        {
            auto split_result = self.split_leaf_in_a_half(path, idx);

            if (split_result.type() == bt::SplitStatus::RIGHT)
            {
                result.leaf_changed = true;
                result.new_idx = idx = split_result.stream_idx();
            }

            auto removed_try2 = self.template ctr_try_remove_stream_entry<0>(path, idx);

            if (!is_success(removed_try2))
            {
                MEMORIA_MAKE_GENERIC_ERROR("BTSS error: second entry removal attempt failed").do_throw();
            }
        }

        self.ctr_update_path(path, 0);

        TreePathT next_path = path;
        auto has_next = self.ctr_get_next_node(next_path, 0);
        if (has_next)
        {
            self.ctr_merge_leaf_nodes(path, next_path);
        }
        else {
            TreePathT prev_path = path;

            auto has_prev = self.ctr_get_prev_node(prev_path, 0);

            if (has_prev)
            {
                auto left_sizes = self.ctr_get_leaf_sizes(prev_path.leaf());
                auto left_merge_result = self.ctr_merge_leaf_nodes(prev_path, path, false);
                if (left_merge_result)
                {
                    path = prev_path;

                    result.leaf_changed = true;
                    result.new_idx += left_sizes[0];
                }
            }
        }

        return result;
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
