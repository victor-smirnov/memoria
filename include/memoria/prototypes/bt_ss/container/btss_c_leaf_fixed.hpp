
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

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss::LeafFixedName)
public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    using CtrSizeT = typename Types::CtrSizeT;

    static const int32_t Streams = Types::Streams;



    MEMORIA_V1_DECLARE_NODE_FN(GetStreamCapacityFn, single_stream_capacity);

public:
    Int32Result ctr_get_leaf_node_capacity(const NodeBaseG& node, int max_hops = 100) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, GetStreamCapacityFn(), max_hops);
    }


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btss::LeafFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS

}
