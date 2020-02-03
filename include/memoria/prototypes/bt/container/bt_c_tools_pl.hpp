
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

#include <memoria/core/container/macros.hpp>
#include <memoria/core/strings/string.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <iostream>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ToolsPLName)
public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Allocator::BlockG                                    BlockG;

    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    static const int32_t Streams                                                = Types::Streams;

    using typename Base::TreePathT;

public:


    void ctr_dump_path(const TreePathT& path, size_t level, std::ostream& out = std::cout, int32_t depth = 100) const noexcept
    {
        auto& self = this->self();

        out << "Path:" << std::endl;

        for (size_t ll = path.size(); ll > level; ll--)
        {
            self.ctr_dump_node(path[ll - 1], out);
        }
    }


public:
    Result<NodeBaseG> ctr_get_node_parent(const TreePathT& path, size_t level) const noexcept
    {
        using ResultT = Result<NodeBaseG>;
        if (MMA_LIKELY(level + 1 < path.size()))
        {
            return ResultT::of(path[level + 1]);
        }
        else {
            return ResultT::make_error(
                        "Invalid tree path parent access. Requesting level = {}, path size = {}",
                        level,
                        path.size()
            );
        }
    }





    Result<NodeBaseG> ctr_get_node_parent_for_update(TreePathT& path, size_t level) const noexcept
    {
        using ResultT = Result<NodeBaseG>;
        if (MMA_LIKELY(level + 1 < path.size()))
        {
            MEMORIA_TRY_VOID(path[level + 1].update());
            return ResultT::of(path[level + 1]);
        }
        else {
            return ResultT::make_error(
                        "Invalid tree path parent access. Requesting level = {}, path size = {}",
                        level,
                        path.size()
            );
        }
    }



public:

    BoolResult ctr_get_next_node(TreePathT& path, size_t level) const noexcept;
    BoolResult ctr_get_prev_node(TreePathT& path, size_t level) const noexcept;

    VoidResult ctr_expect_next_node(TreePathT& path, size_t level) const noexcept
    {
        MEMORIA_TRY(has_next, self().ctr_get_next_node(path, level));
        if (!has_next) {
            return VoidResult::make_error("Next node not found");
        }
        return VoidResult::of();
    }

    VoidResult ctr_expect_prev_node(TreePathT& path, size_t level) const noexcept
    {
        MEMORIA_TRY(has_prev, self().ctr_get_prev_node(path, level));
        if (!has_prev) {
            return VoidResult::make_error("Previous node not found");
        }
        return VoidResult::of();
    }

protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ToolsPLName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
BoolResult M_TYPE::ctr_get_next_node(TreePathT& path, size_t level) const noexcept
{
    using ResultT = BoolResult;

    auto& self = this->self();

    NodeBaseG node = path[level];

    if (!node->is_root())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent(path, level));

        int32_t size = self.ctr_get_node_size(parent, 0);

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, node->id()));

        if (parent_idx < size - 1)
        {
            MEMORIA_TRY(child, self.ctr_get_node_child(parent, parent_idx + 1));
            path[level] = child;
            return BoolResult::of(true);
        }
        else {
            MEMORIA_TRY(has_next_parent, ctr_get_next_node(path, level + 1));

            if (has_next_parent)
            {
                MEMORIA_TRY(child, self.ctr_get_node_child(path[level + 1], 0));
                path[level] = child;
                return ResultT::of(true);
            }
        }
    }

    return ResultT::of(false);
}


M_PARAMS
BoolResult M_TYPE::ctr_get_prev_node(TreePathT& path, size_t level) const noexcept
{
    using ResultT = BoolResult;
    auto& self = this->self();

    NodeBaseG node = path[level];

    if (!node->is_root())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent(path, level));
        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, node->id()));

        if (parent_idx > 0)
        {
            MEMORIA_TRY(child, self.ctr_get_node_child(parent, parent_idx - 1));
            path[level] = child;
            return ResultT::of(true);
        }
        else {
            MEMORIA_TRY(has_prev_parent, ctr_get_prev_node(path, level + 1));

            if (has_prev_parent)
            {
                int32_t node_size = self.ctr_get_node_size(path[level + 1], 0);
                MEMORIA_TRY(child, self.ctr_get_node_child(path[level + 1], node_size - 1));
                path[level] = child;
                return ResultT::of(true);
            }
        }
    }

    return ResultT::of(false);
}






#undef M_TYPE
#undef M_PARAMS

}
