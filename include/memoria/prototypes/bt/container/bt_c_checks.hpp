
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

#include <iostream>

#include <memoria/core/container/logs.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ChecksName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;
    using typename Base::TreePathT;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;

    BoolResult check(void *) const noexcept
    {
        return self().ctr_check_tree();
    }



    VoidResult ctr_check_it() noexcept
    {
        auto& self = this->self();

        self.logger().level() = Logger::_ERROR;

        if (self.ctr_check_tree())
        {
            return VoidResult::of((SBuf() << "Container " << self.name() << " (" << self.type_name_str() << ") check failed").str().c_str());
        }

        return VoidResult::of();
    }

    VoidResult ctr_check_path(const TreePathT& path, size_t level = 0) const noexcept
    {
        auto& self = this->self();
        for (size_t ll = level + 1; ll < path.size(); ll++)
        {
            BlockID child_id = path[ll - 1]->id();
            MEMORIA_TRY_VOID(self.ctr_get_child_idx(path[ll], child_id));
        }
        return VoidResult::of();
    }

    VoidResult ctr_check_same_paths(
            const TreePathT& left_path,
            const TreePathT& right_path,
            size_t level = 0
    ) const noexcept
    {
        if (left_path.size() == right_path.size())
        {
            for (size_t ll = level; ll < left_path.size(); ll++)
            {
                if (left_path[ll] != right_path[ll]) {
                    return VoidResult::make_error(
                                "Path nodes are not equals at the level {} :: {} {}",
                                ll,
                                left_path[ll]->id(),
                                right_path[ll]->id()
                    );
                }
            }
            return VoidResult::of();
        }
        else {
            return VoidResult::make_error(
                        "Path sizes are different: {} {}",
                        left_path.size(),
                        right_path.size()
            );
        }
    }

public:
    BoolResult ctr_check_tree() const noexcept;

    MEMORIA_V1_DECLARE_NODE_FN(CheckContentFn, check);
    bool ctr_check_content(const NodeBaseG& node) const noexcept
    {
        try {
            self().node_dispatcher().dispatch(node, CheckContentFn());
            return false;
        }
        catch (Exception& ex)
        {
            self().ctr_dump_node(node);

            MMA_ERROR(self(), "Node content check failed", ex.message());
            return true;
        }
    }




private:
    VoidResult ctr_check_tree_structure(const NodeBaseG& parent, int32_t parent_idx, const NodeBaseG& node, bool &errors) const noexcept;


    template <typename Node1, typename Node2>
    BoolResult ctr_check_typed_node_content(Node1&& node, Node2&& parent, int32_t parent_idx) const noexcept;

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(CheckTypedNodeContentFn, ctr_check_typed_node_content, BoolResult);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ChecksName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
BoolResult M_TYPE::ctr_check_tree() const noexcept
{
    auto& self = this->self();

    Result<NodeBaseG> root = self.ctr_get_root_node();
    MEMORIA_RETURN_IF_ERROR(root);

    if (root.get())
    {
        bool errors = false;
        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_check_tree_structure(NodeBaseG(), 0, root.get(), errors));
        return BoolResult::of(errors);
    }
    else {
        MMA_ERROR(self, "No root node for container");
        return BoolResult::of(true);
    }
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
VoidResult M_TYPE::ctr_check_tree_structure(const NodeBaseG& parent, int32_t parent_idx, const NodeBaseG& node, bool &errors) const noexcept
{
    auto& self = this->self();

    errors = self.ctr_check_content(node) || errors;

    if (!node->is_root())
    {
        BoolResult res_vv = self.tree_dispatcher().dispatchTree(parent, node, CheckTypedNodeContentFn(self), parent_idx);
        MEMORIA_RETURN_IF_ERROR(res_vv);

        errors = res_vv.get() || errors;

        if (!node->is_leaf())
        {
            int32_t children = self.ctr_get_node_size(node, 0);

            if (children == 0 && !node->is_root())
            {
                errors = true;
                MMA_ERROR(self, "children == 0 for non-root node", node->id());
                self.ctr_dump_node(node);
            }
        }
    }

    if (!node->is_leaf())
    {
        int32_t children = self.ctr_get_node_size(node, 0);

        // TODO: check children IDs findability;

        for (int32_t c = 0; c < children; c++)
        {
            BlockID child_id = self.ctr_get_child_id(node, c);

            MEMORIA_TRY(child, self.ctr_get_node_child(node, c));

            if (child->id() != child_id)
            {
                errors = true;
                MMA_ERROR(self, "child.id != child_id", child->id(), child->id(), child_id);
            }

            return self.ctr_check_tree_structure(node, c, child, errors);
        }
    }

    return VoidResult::of();
}

M_PARAMS
template <typename Node1, typename Node2>
BoolResult M_TYPE::ctr_check_typed_node_content(Node1&& parent, Node2&& node, int32_t parent_idx) const noexcept
{
    bool errors = false;

    BranchNodeEntry sums;

    node.max(sums);

    BranchNodeEntry keys = parent.keysAt(parent_idx);

    if (sums != keys)
    {
        MMA_ERROR(
                self(),
                "Invalid parent-child nodes chain",
                (SBuf() << sums).str(),
                (SBuf() << keys).str(),
                "for node.id=",
                node.node()->id(),
                "parent.id=",
                parent.node()->id(),
                "parent_idx",
                parent_idx
        );

        errors = true;
    }

    return BoolResult::of(errors);
}

#undef M_TYPE
#undef M_PARAMS

}
