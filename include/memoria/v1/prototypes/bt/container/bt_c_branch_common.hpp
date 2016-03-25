
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

using namespace v1::bt;
using namespace v1::core;

using namespace std;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::BranchCommonName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>             SplitFn;

protected:
    static const Int Streams = Types::Streams;

    void newRootP(NodeBaseG& root);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetNonLeafCapacityFn, capacity, Int);
    Int getBranchNodeCapacity(const NodeBaseG& node, UBigInt active_streams) const
    {
        return BranchDispatcher::dispatch(node, GetNonLeafCapacityFn(), active_streams);
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, BranchNodeEntry);
    BranchNodeEntry splitBranchNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::BranchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::BranchNodeEntry M_TYPE::splitBranchNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at)
{
    auto& self = this->self();

    BranchNodeEntry accum = BranchDispatcher::dispatch(src, tgt, SplitNodeFn(), split_at);

    self.updateChildren(tgt);

    return accum;
}


M_PARAMS
void M_TYPE::newRootP(NodeBaseG& root)
{
    auto& self = this->self();

    self.updatePageG(root);

    NodeBaseG new_root = self.createNode(root->level() + 1, true, false, root->page_size());

    UBigInt root_active_streams = self.getActiveStreams(root);
    self.layoutBranchNode(new_root, root_active_streams);

    self.copyRootMetadata(root, new_root);

    self.root2Node(root);

    BranchNodeEntry max = self.max(root);

    self.insertToBranchNodeP(new_root, 0, max, root->id());

    root->parent_id()  = new_root->id();
    root->parent_idx() = 0;

    self.set_root(new_root->id());
}



#undef M_TYPE
#undef M_PARAMS

}}