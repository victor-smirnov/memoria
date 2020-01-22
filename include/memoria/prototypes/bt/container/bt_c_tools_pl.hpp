
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

public:


    void ctr_dump_path(NodeBaseG node, std::ostream& out = std::cout, int32_t depth = 100) const noexcept
    {
        auto& self = this->self();

        out << "Path:" << std::endl;

        self.ctr_dump_node(node, out);

        while (!node->is_root() && node->level() < depth)
        {
            node = self.ctr_get_node_parent(node).get_or_terminate();
            self.ctr_dump_node(node, out);
        }
    }


public:

    Result<NodeBaseG> ctr_get_node_parent(const NodeBaseG& node) const noexcept
    {
        auto& self = this->self();
        return static_cast_block<NodeBaseG>(self.store().getBlock(node->parent_id()));
    }

    Result<NodeBaseG> ctr_get_node_parent_for_update(const NodeBaseG& node) const noexcept
    {
        auto& self = this->self();
        return static_cast_block<NodeBaseG>(self.store().getBlockForUpdate(node->parent_id()));
    }


public:

    Result<NodeBaseG> ctr_get_next_node(NodeBaseG& node) const noexcept;
    Result<NodeBaseG> ctr_get_prev_node(NodeBaseG& node) const noexcept;

protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ToolsPLName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
Result<typename M_TYPE::NodeBaseG> M_TYPE::ctr_get_next_node(NodeBaseG& node) const noexcept
{
    using ResultT = Result<NodeBaseG>;

    auto& self = this->self();

    if (!node->is_root())
    {
        Result<NodeBaseG> parent = self.ctr_get_node_parent(node);
        MEMORIA_RETURN_IF_ERROR(parent);

        int32_t size = self.ctr_get_node_size(parent.get(), 0);

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent.get(), node->id()));

        if (parent_idx < size - 1)
        {
            return self.ctr_get_node_child(parent.get(), parent_idx + 1);
        }
        else {
            Result<NodeBaseG> target_parent = ctr_get_next_node(parent.get());
            MEMORIA_RETURN_IF_ERROR(target_parent);

            if (target_parent.get().isSet())
            {
                return self.ctr_get_node_child(target_parent.get(), 0);
            }
            else {
                return target_parent;
            }
        }
    }
    else {
        return ResultT::of();
    }
}


M_PARAMS
Result<typename M_TYPE::NodeBaseG> M_TYPE::ctr_get_prev_node(NodeBaseG& node) const noexcept
{
    using ResultT = Result<NodeBaseG>;
    auto& self = this->self();

    if (!node->is_root())
    {
        Result<NodeBaseG> parent = self.ctr_get_node_parent(node);
        MEMORIA_RETURN_IF_ERROR(parent);

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent.get(), node->id()));

        if (parent_idx > 0)
        {
            return self.ctr_get_node_child(parent.get(), parent_idx - 1);
        }
        else {
            Result<NodeBaseG> target_parent = ctr_get_prev_node(parent.get());

            if (target_parent.get().isSet())
            {
                int32_t node_size = self.ctr_get_node_size(target_parent.get(), 0);
                return self.ctr_get_node_child(target_parent.get(), node_size - 1);
            }
            else {
                return target_parent;
            }
        }
    }
    else {
        return ResultT::of();
    }
}






#undef M_TYPE
#undef M_PARAMS

}
