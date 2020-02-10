
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <vector>

namespace memoria {

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

    typedef std::function<Result<BranchNodeEntry> (NodeBaseG&, NodeBaseG&)>             SplitFn;

    using typename Base::TreePathT;

public:
    static const int32_t Streams = Types::Streams;

    VoidResult ctr_create_new_root_block(TreePathT& path) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetNonLeafCapacityFn, capacity, int32_t);
    int32_t ctr_get_branch_node_capacity(const NodeBaseG& node, uint64_t active_streams) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, GetNonLeafCapacityFn(), active_streams);
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, OpStatus);
    Result<OpStatus> ctr_split_branch_node(NodeBaseG& src, NodeBaseG& tgt, int32_t split_at) noexcept;

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
Result<OpStatus> M_TYPE::ctr_split_branch_node(NodeBaseG& src, NodeBaseG& tgt, int32_t split_at) noexcept
{
    using ResultT = Result<OpStatus>;
    auto& self = this->self();

    if (isFail(self.branch_dispatcher().dispatch(src, tgt, SplitNodeFn(), split_at))) {
        return ResultT::of(OpStatus::FAIL);
    }

    return ResultT::of(OpStatus::OK);
}


M_PARAMS
VoidResult M_TYPE::ctr_create_new_root_block(TreePathT& path) noexcept
{
    auto& self = this->self();

    NodeBaseG root = path.root();

    MEMORIA_TRY(new_root, self.ctr_create_node(root->level() + 1, true, false, root->header().memory_block_size()));

    uint64_t root_active_streams = self.ctr_get_active_streams(root);
    MEMORIA_TRY_VOID(self.ctr_layout_branch_node(new_root, root_active_streams));
    MEMORIA_TRY_VOID(self.ctr_copy_root_metadata(root, new_root));
    MEMORIA_TRY_VOID(self.ctr_root_to_node(root));

    BranchNodeEntry max = self.ctr_get_node_max_keys(root);

    path.add_root(new_root);

    MEMORIA_TRY(status, self.ctr_insert_to_branch_node(path, new_root->level(), 0, max, root->id()));

    if (isFail(status)) {
        return MEMORIA_MAKE_GENERIC_ERROR("PackedOOMException");
    }

    return self.set_root(new_root->id());
}

#undef M_TYPE
#undef M_PARAMS

}
