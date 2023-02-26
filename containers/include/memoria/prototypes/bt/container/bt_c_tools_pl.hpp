
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

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::TreePathT;

    void ctr_dump_path(const TreePathT& path, size_t level, std::ostream& out = std::cout) const
    {
        auto& self = this->self();

        out << "Path:" << std::endl;

        for (size_t ll = path.size(); ll > level; ll--)
        {
            self.ctr_dump_node(path[ll - 1], out);
        }
    }


public:
    TreeNodeConstPtr ctr_get_node_parent(const TreePathT& path, size_t level) const
    {
        if (MMA_LIKELY(level + 1 < path.size()))
        {
            return path[level + 1];
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Invalid tree path parent access. Requesting level = {}, path size = {}",
                        level,
                        path.size()
            ).do_throw();
        }
    }





    TreeNodePtr ctr_get_node_parent_for_update(TreePathT& path, size_t level) const
    {
        if (MMA_LIKELY(level + 1 < path.size()))
        {
            path[level + 1].update();
            return path[level + 1].as_mutable();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Invalid tree path parent access. Requesting level = {}, path size = {}",
                        level,
                        path.size()
            ).do_throw();
        }
    }



public:

    bool ctr_get_next_node(TreePathT& path, size_t level) const;
    bool ctr_get_prev_node(TreePathT& path, size_t level) const;

    void ctr_expect_next_node(TreePathT& path, size_t level) const
    {
        auto has_next = self().ctr_get_next_node(path, level);
        if (!has_next) {
            MEMORIA_MAKE_GENERIC_ERROR("Next node not found").do_throw();
        }
    }

    void ctr_expect_prev_node(TreePathT& path, size_t level) const
    {
        auto has_prev = self().ctr_get_prev_node(path, level);
        if (!has_prev) {
            MEMORIA_MAKE_GENERIC_ERROR("Previous node not found").do_throw();
        }
    }

protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ToolsPLName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
bool M_TYPE::ctr_get_next_node(TreePathT& path, size_t level) const
{
    auto& self = this->self();

    TreeNodeConstPtr node = path[level];

    if (!node->is_root())
    {
        auto parent = self.ctr_get_node_parent(path, level);

        auto size = self.ctr_get_node_size(parent, 0);

        size_t parent_idx = self.ctr_get_child_idx(parent, node->id());
        if (parent_idx < size - 1)
        {
            auto child = self.ctr_get_node_child(parent, parent_idx + 1);
            path[level] = child;
            return true;
        }
        else {
            auto has_next_parent = ctr_get_next_node(path, level + 1);

            if (has_next_parent)
            {
                auto child = self.ctr_get_node_child(path[level + 1], 0);
                path[level] = child;
                return true;
            }
        }
    }

    return false;
}


M_PARAMS
bool M_TYPE::ctr_get_prev_node(TreePathT& path, size_t level) const
{
    auto& self = this->self();

    TreeNodeConstPtr node = path[level];

    if (!node->is_root())
    {
        auto parent = self.ctr_get_node_parent(path, level);
        auto parent_idx = self.ctr_get_child_idx(parent, node->id());

        if (parent_idx > 0)
        {
            auto child = self.ctr_get_node_child(parent, parent_idx - 1);
            path[level] = child;
            return true;
        }
        else {
            auto has_prev_parent = ctr_get_prev_node(path, level + 1);

            if (has_prev_parent)
            {
                auto node_size = self.ctr_get_node_size(path[level + 1], 0);
                auto child = self.ctr_get_node_child(path[level + 1], node_size - 1);
                path[level] = child;
                return true;
            }
        }
    }

    return false;
}

#undef M_TYPE
#undef M_PARAMS

}
