
// Copyright 2013 Victor Smirnov
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

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(seq_dense::CtrInsertVariableName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    //==========================================================================================

    struct InsertBufferIntoLeafFn
    {
        OpStatus status_{OpStatus::OK};

        template <typename NTypes, typename LeafPosition, typename Buffer>
        void treeNode(bt::LeafNode<NTypes>* node, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
        {
            node->processAll(*this, pos, start, size, buffer);
        }

        template <typename StreamType, typename LeafPosition, typename Buffer>
        void stream(StreamType* obj, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
        {
            if (isOk(status_)) {
                status_ <<= obj->insert(buffer, pos, start, size);
            }
        }
    };


    template <typename LeafPosition, typename Buffer>
    MMA1_NODISCARD bool doInsertBufferIntoLeaf(NodeBaseG& leaf, BlockUpdateMgr& mgr, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
    {
        InsertBufferIntoLeafFn fn;
        self().leaf_dispatcher().dispatch(leaf, fn, pos, start, size - start, buffer);

        if (isOk(fn.status_)) {
            mgr.checkpoint(leaf);
            return true;
        }
        else {
            mgr.restoreNodeState();
            return false;
        }
    }




MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(seq_dense::CtrInsertVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
