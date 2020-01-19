
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

    typedef std::function<Result<BranchNodeEntry> (NodeBaseG&, NodeBaseG&)>             SplitFn;

public:
    static const int32_t Streams = Types::Streams;

    VoidResult ctr_create_new_root_block(NodeBaseG& root) noexcept;

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

    auto res = self.ctr_update_children(tgt);
    MEMORIA_RETURN_IF_ERROR(res);

    return ResultT::of(OpStatus::OK);
}


M_PARAMS
VoidResult M_TYPE::ctr_create_new_root_block(NodeBaseG& root) noexcept
{
    auto& self = this->self();

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(root));

    Result<NodeBaseG> new_root = self.ctr_create_node(root->level() + 1, true, false, root->header().memory_block_size());
    MEMORIA_RETURN_IF_ERROR(new_root);

    uint64_t root_active_streams = self.ctr_get_active_streams(root);
    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_layout_branch_node(new_root.get(), root_active_streams));

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_copy_root_metadata(root, new_root.get()));

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_root_to_node(root));

    BranchNodeEntry max = self.ctr_get_node_max_keys(root);

    Result<OpStatus> status = self.ctr_insert_to_branch_node(new_root.get(), 0, max, root->id());
    MEMORIA_RETURN_IF_ERROR(status);

    if (isFail(status.get())) {
        return VoidResult::make_error("PackedOOMException");
    }

    root->parent_id()  = new_root.get()->id();
    root->parent_idx() = 0;

    return self.set_root(new_root.get()->id());

}



#undef M_TYPE
#undef M_PARAMS

}}
