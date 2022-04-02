
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

MEMORIA_V1_CONTAINER_PART_BEGIN(seq_dense::CtrInsertName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;



    typedef typename Types::TreeNodePtr                                         TreeNodePtr;
    typedef typename Base::Iterator                                             Iterator;



    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;
    typedef typename Types::CtrSizeT                                            CtrSizeT;

//    struct InsertIntoLeafFn {
//
//        template <int32_t Idx, typename SeqTypes>
//        void stream(PkdFSSeq<SeqTypes>* seq, int32_t idx, int32_t symbol, BranchNodeEntry* delta)
//        {
//            MEMORIA_V1_ASSERT_TRUE(seq != nullptr);
//
//            typedef PkdFSSeq<SeqTypes>                  Seq;
//            typedef typename Seq::Value                 Symbol;
//
//            seq->insert(idx, 1, [=]() -> Symbol {
//                return symbol;
//            });
//
//            std::get<Idx>(*delta)[0]++;
//            std::get<Idx>(*delta)[symbol + 1]++;
//        }
//
//
//        template <typename NTypes>
//        void treeNode(LeafNode<NTypes>* node, int32_t stream, int32_t idx, int32_t symbol, BranchNodeEntry* delta)
//        {
//            node->layout(1);
//            node->process(stream, *this, idx, symbol, delta);
//        }
//    };
//
//
//
//    bool insertIntoLeaf(TreeNodePtr& leaf, int32_t idx, int32_t symbol, BranchNodeEntry& indexes)
//    {
//        auto& self = this->self();
//
//        BlockUpdateMgr mgr(self);
//
//        mgr.add(leaf);
//
//        try {
//            self().leaf_dispatcher().dispatch(leaf, InsertIntoLeafFn(), 0, idx, symbol, &indexes);
//            return true;
//        }
//        catch (PackedOOMException& e)
//        {
//            Clear(indexes);
//            mgr.rollback();
//            return false;
//        }
//    }
//

    void insert_symbol(CtrSizeT idx, int32_t symbol)
    {
        auto& self  = this->self();
        auto iter   = self.seek(idx);

        iter->insert_symbol(symbol);
    }
//
//    void insert(Iterator& iter, int32_t symbol)
//    {
//        auto& self  = this->self();
//        auto& leaf  = iter.iter_leaf();
//        int32_t& idx    = iter.iter_local_pos();
//
//        self.ctr_update_block_guard(leaf);
//
//        BranchNodeEntry sums;
//
//        if (self.insertIntoLeaf(leaf, idx, symbol, sums))
//        {
//            self.update_parent(leaf, sums);
//        }
//        else
//        {
//            int32_t size        = iter.iter_leaf_size(0);
//            int32_t split_idx   = size/2;
//
//            auto right = self.ctr_split_leaf(leaf, Position::create(0, split_idx));
//
//            if (idx > split_idx)
//            {
//                leaf = right;
//                idx -= split_idx;
//            }
//
//            bool result = self.insertIntoLeaf(leaf, idx, symbol, sums);
//            MEMORIA_V1_ASSERT_TRUE(result);
//            self.update_parent(leaf, sums);
//        }
//
//        iter++;
//    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(seq_dense::CtrInsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
