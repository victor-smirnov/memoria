
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

#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ChecksName)

    using typename Base::BlockID;
    using typename Base::TreePathT;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::BranchNodeEntry;



    void check(const CheckResultConsumerFn& fn) {
        self().ctr_check_tree(fn);
    }

    void ctr_check_it(const CheckResultConsumerFn& fn)
    {
        self().ctr_check_tree(fn);
    }

    void ctr_check_path(const TreePathT& path, size_t level = 0) const
    {
        auto& self = this->self();
        for (size_t ll = level + 1; ll < path.size(); ll++)
        {
            BlockID child_id = path[ll - 1]->id();

            // FIXME: handle returned index!
            self.ctr_get_child_idx(path[ll], child_id);
        }
    }

    void ctr_check_same_paths(
            const TreePathT& left_path,
            const TreePathT& right_path,
            size_t level = 0
    ) const
    {
        if (left_path.size() == right_path.size())
        {
            for (size_t ll = level; ll < left_path.size(); ll++)
            {
                if (left_path[ll] != right_path[ll]) {
                    MEMORIA_MAKE_GENERIC_ERROR(
                                "Path nodes are not equal at the level {} :: {} {}",
                                ll,
                                left_path[ll]->id(),
                                right_path[ll]->id()
                    ).do_throw();
                }
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Path sizes are different: {} {}",
                        left_path.size(),
                        right_path.size()
            ).do_throw();
        }
    }

public:
    void ctr_check_tree(const CheckResultConsumerFn& fn) const ;

    MEMORIA_V1_DECLARE_NODE_FN(CheckContentFn, check);
    void ctr_check_content(const TreeNodeConstPtr& node, const CheckResultConsumerFn& fn) const
    {
        self().node_dispatcher().dispatch(node, CheckContentFn(), fn).get_or_throw();
    }

private:
    void ctr_check_tree_structure(
            const TreeNodeConstPtr& parent,
            int32_t parent_idx,
            const TreeNodeConstPtr& node,
            const CheckResultConsumerFn& fn
    ) const ;


    template <typename Node1, typename Node2>
    VoidResult ctr_check_typed_node_content(
            Node1&& node,
            Node2&& parent,
            int32_t parent_idx,
            const CheckResultConsumerFn& fn
    ) const ;

    MEMORIA_V1_CONST_FN_WRAPPER(CheckTypedNodeContentFn, ctr_check_typed_node_content);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ChecksName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::ctr_check_tree(const CheckResultConsumerFn& fn) const
{
    auto& self = this->self();

    auto root = self.ctr_get_root_node();
    if (root)
    {
        self.ctr_check_tree_structure(TreeNodeConstPtr(), 0, root, fn);
    }
    else {
        fn(CheckSeverity::ERROR, make_string_document("No root node is specified for container {}", self.name()));
    }
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::ctr_check_tree_structure(
        const TreeNodeConstPtr& parent,
        int32_t parent_idx,
        const TreeNodeConstPtr& node,
        const CheckResultConsumerFn& fn
) const
{
    auto& self = this->self();

    self.ctr_check_content(node, fn);

    if (!node->is_root())
    {
        self.tree_dispatcher().dispatchTree(parent, node, CheckTypedNodeContentFn(self), parent_idx, fn).get_or_throw();

        if (!node->is_leaf())
        {
            auto children = self.ctr_get_node_size(node, 0);
            if (children == 0 && !node->is_root())
            {
                fn(CheckSeverity::ERROR, make_string_document("children == 0 for non-root node, node = {}", self.ctr_describe_node(node)));
            }
        }
    }

    if (!node->is_leaf())
    {
        auto children = self.ctr_get_node_size(node, 0);

        // TODO: check children IDs findability;
        for (int32_t c = 0; c < children; c++)
        {
            auto child_id = self.ctr_get_child_id(node, c);
            auto child = self.ctr_get_node_child(node, c);

            if (child->id() != child_id)
            {
                fn(CheckSeverity::ERROR, make_string_document("child.id != child_id :: {} {} {}", child->id(), child->id(), child_id));
            }

            return self.ctr_check_tree_structure(node, c, child, fn);
        }
    }
}

M_PARAMS
template <typename Node1, typename Node2>
VoidResult M_TYPE::ctr_check_typed_node_content(
        Node1&& parent,
        Node2&& node,
        int32_t parent_idx,
        const CheckResultConsumerFn& fn
) const
{
    BranchNodeEntry sums;

    node.max(sums).get_or_throw();

    auto keys = parent.keysAt(parent_idx).get_or_throw();

    if (sums != keys)
    {
        fn(
            CheckSeverity::ERROR,
            make_string_document(
                        "Invalid parent-child nodes chain :: {} {} for node.id={} parent.id={}, parent_idx={}",
                        (SBuf() << sums).str(),
                        (SBuf() << keys).str(),
                        node.node()->id(),
                        parent.node()->id(),
                        parent_idx
            )
        );
    }

    return VoidResult::of();
}

#undef M_TYPE
#undef M_PARAMS

}
