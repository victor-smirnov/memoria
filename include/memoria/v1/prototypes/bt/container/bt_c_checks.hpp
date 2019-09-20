
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

#include <memoria/v1/core/container/logs.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ChecksName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;

    bool check(void *data) const
    {
        return self().ctr_check_tree();
    }



    void ctr_check_it()
    {
        auto& self = this->self();

        self.logger().level() = Logger::_ERROR;

        if (self.ctr_check_tree())
        {
            throw Exception(MA_SRC, SBuf()<<"Container "<<self.name()<<" ("<<self.type_name_str()<<") check failed");
        }
    }

public:
    bool ctr_check_tree() const;

    MEMORIA_V1_DECLARE_NODE_FN(CheckContentFn, check);
    bool ctr_check_content(const NodeBaseG& node) const
    {
        try {
            self().node_dispatcher().dispatch(node, CheckContentFn());
            return false;
        }
        catch (Exception& ex)
        {
            self().ctr_dump_node(node);

            MMA1_ERROR(self(), "Node content check failed", ex.message());
            return true;
        }
    }



private:
    void ctr_check_tree_structure(const NodeBaseG& parent, int32_t parent_idx, const NodeBaseG& node, bool &errors) const;


    template <typename Node1, typename Node2>
    bool ctr_check_typed_node_content(Node1&& node, Node2&& parent, int32_t parent_idx) const;

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(CheckTypedNodeContentFn, ctr_check_typed_node_content, bool);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ChecksName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::ctr_check_tree() const
{
    auto& self = this->self();

    NodeBaseG root = self.ctr_get_root_node();
    if (root)
    {
        bool errors = false;
        self.ctr_check_tree_structure(NodeBaseG(), 0, root, errors);
        return errors;
    }
    else {
        MMA1_ERROR(self, "No root node for container");
        return true;
    }
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::ctr_check_tree_structure(const NodeBaseG& parent, int32_t parent_idx, const NodeBaseG& node, bool &errors) const
{
    auto& self = this->self();

    errors = self.ctr_check_content(node) || errors;

    if (!node->is_root())
    {
        errors = self.tree_dispatcher().dispatchTree(parent, node, CheckTypedNodeContentFn(self), parent_idx) || errors;

        if (!node->is_leaf())
        {
            int32_t children = self.ctr_get_node_size(node, 0);

            if (children == 0 && !node->is_root())
            {
                errors = true;
                MMA1_ERROR(self, "children == 0 for non-root node", node->id());
                self.ctr_dump_node(node);
            }
        }
    }

    if (!node->is_leaf())
    {
        int32_t children = self.ctr_get_node_size(node, 0);

        for (int32_t c = 0; c < children; c++)
        {
            BlockID child_id = self.ctr_get_child_id(node, c);

            NodeBaseG child = self.ctr_get_node_child(node, c);

            if (child->id() != child_id)
            {
                errors = true;
                MMA1_ERROR(self, "child.id != child_id", child->id(), child->id(), child_id);
            }

            if (child->parent_idx() != c)
            {
                errors = true;
                MMA1_ERROR(self, "child.parent_idx != idx", child->parent_idx(), c, node->id(), child->id());
                std::cout << "parent_idx: " << child->parent_idx() << " " << c << std::endl;
            }

            if (child->parent_id() != node->id())
            {
                errors = true;
                MMA1_ERROR(self, "child.parent_id != node.id", child->parent_id(), node->id());
                std::cout << "parent_idx: " << child->parent_id() << " " << node->id() << std::endl;
            }

            self.ctr_check_tree_structure(node, c, child, errors);
        }
    }
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::ctr_check_typed_node_content(Node1&& parent, Node2&& node, int32_t parent_idx) const
{
    bool errors = false;

    BranchNodeEntry sums;

    node.max(sums);

    BranchNodeEntry keys = parent.keysAt(parent_idx);

    if (sums != keys)
    {
        MMA1_ERROR(
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


    return errors;
}

#undef M_TYPE
#undef M_PARAMS

}}
