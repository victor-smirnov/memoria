
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
#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchCommonName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;
    
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    typedef std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>             SplitFn;

public:
    static const int32_t Streams = Types::Streams;

    void ctr_create_new_root_block(NodeBaseG& root);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetNonLeafCapacityFn, capacity, int32_t);
    int32_t ctr_get_branch_node_capacity(const NodeBaseG& node, uint64_t active_streams) const
    {
        return self().branch_dispatcher().dispatch(node, GetNonLeafCapacityFn(), active_streams);
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, OpStatus);
    OpStatus ctr_split_branch_node(NodeBaseG& src, NodeBaseG& tgt, int32_t split_at);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
OpStatus M_TYPE::ctr_split_branch_node(NodeBaseG& src, NodeBaseG& tgt, int32_t split_at)
{
    auto& self = this->self();

    if (isFail(self.branch_dispatcher().dispatch(src, tgt, SplitNodeFn(), split_at))) {
        return OpStatus::FAIL;
    }

    self.ctr_update_children(tgt);

    return OpStatus::OK;
}


M_PARAMS
void M_TYPE::ctr_create_new_root_block(NodeBaseG& root)
{
    auto& self = this->self();

    self.ctr_update_block_guard(root);

    NodeBaseG new_root = self.ctr_create_node(root->level() + 1, true, false, root->header().memory_block_size());

    uint64_t root_active_streams = self.ctr_get_active_streams(root);
    self.ctr_layout_branch_node(new_root, root_active_streams);

    self.ctr_copy_root_metadata(root, new_root);

    self.ctr_root_to_node(root);

    BranchNodeEntry max = self.ctr_get_node_max_keys(root);

    OOM_THROW_IF_FAILED(self.ctr_insert_to_branch_node(new_root, 0, max, root->id()), MMA1_SRC);

    root->parent_id()  = new_root->id();
    root->parent_idx() = 0;

    self.set_root(new_root->id()).terminate_if_error();
}



#undef M_TYPE
#undef M_PARAMS

}}
