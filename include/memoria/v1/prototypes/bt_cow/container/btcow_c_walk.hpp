
// Copyright 2017 Victor Smirnov
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
#include <memoria/v1/core/tools/config.hpp>


namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(btcow::WalkName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;


    void walkTree(ContainerWalker* walker)
    {
        auto& self = this->self();

        NodeBaseG root = self.getRoot();

        walker->beginCtr(
                        TypeNameFactory<typename Types::ContainerTypeName>::name().c_str(),
                        self.name(),
                        root->id()
                );

        this->traverseTree(0, root, walker);

        walker->endCtr();
    }

    void beginNode(int32_t parent_idx, const NodeBaseG& node, ContainerWalker* walker)
    {
        if (node->is_root())
        {
            if (node->is_leaf())
            {
                walker->rootLeaf(parent_idx, node.page());
            }
            else {
                walker->beginRoot(parent_idx, node.page());
            }
        }
        else if (node->is_leaf())
        {
            walker->leaf(parent_idx, node.page());
        }
        else {
            walker->beginNode(parent_idx, node.page());
        }
    }

    void endNode(const NodeBaseG& node, ContainerWalker* walker)
    {
        if (node->is_root())
        {
            if (!node->is_leaf())
            {
                walker->endRoot();
            }
        }
        else if (!node->is_leaf())
        {
            walker->endNode();
        }
    }

private:

    void traverseTree(int32_t parent_idx, const NodeBaseG& node, ContainerWalker* walker)
    {
        auto& self = this->self();

        self.beginNode(parent_idx, node, walker);

        if (!node->is_leaf())
        {
            self.forAllIDs(node, 0, self.getNodeSize(node, 0), [&self, walker](const ID& id, int32_t idx)
            {
                NodeBaseG child = self.allocator().getBlock(id, self.master_name());

                self.traverseTree(idx, child, walker);
            });
        }

        self.endNode(node, walker);
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btcow::WalkName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}}
