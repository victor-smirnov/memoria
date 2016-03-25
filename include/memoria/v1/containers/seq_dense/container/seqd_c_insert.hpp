
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

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <memoria/v1/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/v1/containers/seq_dense/seqd_names.hpp>

#include <memoria/v1/core/tools/static_array.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::seq_dense::CtrInsertName)
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

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

//    struct InsertIntoLeafFn {
//
//        template <Int Idx, typename SeqTypes>
//        void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Int symbol, BranchNodeEntry* delta)
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
//        void treeNode(LeafNode<NTypes>* node, Int stream, Int idx, Int symbol, BranchNodeEntry* delta)
//        {
//            node->layout(1);
//            node->process(stream, *this, idx, symbol, delta);
//        }
//    };
//
//
//
//    bool insertIntoLeaf(NodeBaseG& leaf, Int idx, Int symbol, BranchNodeEntry& indexes)
//    {
//        auto& self = this->self();
//
//        PageUpdateMgr mgr(self);
//
//        mgr.add(leaf);
//
//        try {
//            LeafDispatcher::dispatch(leaf, InsertIntoLeafFn(), 0, idx, symbol, &indexes);
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

    void insert_symbol(CtrSizeT idx, Int symbol)
    {
        auto& self  = this->self();
        auto iter   = self.seek(idx);

        iter->insert_symbol(symbol);
    }
//
//    void insert(Iterator& iter, Int symbol)
//    {
//        auto& self  = this->self();
//        auto& leaf  = iter.leaf();
//        Int& idx    = iter.idx();
//
//        self.updatePageG(leaf);
//
//        BranchNodeEntry sums;
//
//        if (self.insertIntoLeaf(leaf, idx, symbol, sums))
//        {
//            self.update_parent(leaf, sums);
//        }
//        else
//        {
//            Int size        = iter.leaf_size(0);
//            Int split_idx   = size/2;
//
//            auto right = self.split_leaf_p(leaf, Position::create(0, split_idx));
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

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::seq_dense::CtrInsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
