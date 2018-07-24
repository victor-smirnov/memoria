
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

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_tools.hpp>
#include <memoria/v1/containers/seq_dense/seqd_walkers.hpp>


#include <memoria/v1/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(louds::CtrRemoveName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;

    static const int32_t Streams                                                    = Types::Streams;


    struct RemoveFromLeafFn {
        int32_t label_idx_ = -1;


        void stream(StreamSize* obj, int32_t idx)
        {
            obj->remove(idx, idx + 1);
        }

        template <typename SeqTypes>
        void stream(PkdFSSeq<SeqTypes>* seq, int32_t idx)
        {
            MEMORIA_V1_ASSERT_TRUE(seq != nullptr);

            int32_t sym = seq->symbol(idx);

            if (sym) {
                label_idx_ = seq->rank(idx, 1);
            }
            else {
                label_idx_ = -1;
            }

            seq->remove(idx, idx + 1);
        }

        template <typename StreamTypes>
        void stream(PackedFSEArray<StreamTypes>* labels, int32_t idx)
        {
            labels->remove(idx, idx + 1);
        }

        template <typename StreamTypes>
        void stream(PkdVQTree<StreamTypes>* sizes, int32_t idx)
        {
            sizes->remove(idx, idx + 1);
        }

        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, int32_t idx)
        {
            node->template processSubstreams<IntList<0>>(*this, idx);

            if (label_idx_ >= 0)
            {
                node->template processSubstreams<IntList<1>>(*this, label_idx_);
            }
        }
    };

    auto removeFromLeaf(NodeBaseG& leaf, int32_t idx)
    {
        RemoveFromLeafFn fn;
        LeafDispatcher::dispatch(leaf, fn, idx);
    }

    void remove(CtrSizeT idx)
    {
        auto& self  = this->self();
        auto iter   = self.seek(idx);

        self.remove(*iter.get());
    }

    void remove(Iterator& iter)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();
        int32_t& idx    = iter.local_pos();

        removeFromLeaf(leaf, idx);

        self.update_path(leaf);

        self.mergeLeafWithRightSibling(leaf);
    }




    void removeLeaf(const LoudsNode& node)
    {
        auto& self = this->self();

        auto iter = self.findNode(node);

        MEMORIA_V1_ASSERT_TRUE(iter->symbol() == 1);

        iter->firstChild();

        MEMORIA_V1_ASSERT_TRUE(iter->symbol() == 0);

        iter->remove();

        iter = self.findNode(node);
        iter->remove();
    }

    void removeLeaf(Iterator& iter)
    {
        auto& self = this->self();

        CtrSizeT idx = iter.pos();

        MEMORIA_V1_ASSERT_TRUE(iter.symbol() == 1);

        iter.firstChild();

        if (iter.symbol() != 0)
        {
            iter.dumpPath();
        }

        MEMORIA_V1_ASSERT_TRUE(iter.symbol() == 0);

        iter.remove();

        iter = *self.seek(idx).get();
        iter.remove();
    }

MEMORIA_V1_CONTAINER_PART_END

}}
