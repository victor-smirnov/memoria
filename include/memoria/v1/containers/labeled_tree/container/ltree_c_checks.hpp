
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

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::louds::CtrChecksName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;

    static const int32_t Streams                                                    = Types::Streams;


    struct CheckContentFn {

        const MyType&   ctr_;
        int32_t             rank1_;
        bool            errors_     = false;
        bool            has_seq_;
        ID              id_;
        bool            root_;

        CheckContentFn(const MyType& ctr, ID id, bool root):
            ctr_(ctr),
            id_(id),
            root_(root)
        {}

        template <int32_t Idx, typename SeqTypes>
        void stream(const PkdFSSeq<SeqTypes>* seq)
        {
            if (seq != nullptr)
            {
                rank1_ = seq->rank(1);
            }
            else {
                MEMORIA_ERROR(ctr_, "Node", id_, "has no bitmap");
                errors_ = true;
            }

            has_seq_ = seq != nullptr;
        }

        template <int32_t Idx, typename StreamType>
        void checkLabelsSize(const StreamType* labels)
        {
            if (has_seq_)
            {
                if (labels != nullptr)
                {
                    if (labels->size() != rank1_)
                    {
                        MEMORIA_ERROR(ctr_, "Node", id_, "has stream", Idx, "with invalid size. Rank=", rank1_,
                                            "size=",labels->size());

                        errors_ = true;
                    }
                }
                else if (rank1_ > 0)
                {
                    MEMORIA_ERROR(ctr_, "Node", id_, "has positive rank, but stream", Idx, "is empty. Rank=", rank1_);
                }
            }
        }


        template <int32_t Idx, typename StreamTypes>
        void stream(const PackedFSEArray<StreamTypes>* labels)
        {
            checkLabelsSize<Idx>(labels);
        }

        template <int32_t Idx, typename StreamTypes>
        void stream(const PkdVQTree<StreamTypes>* labels)
        {
            checkLabelsSize<Idx>(labels);
        }

        template <int32_t Idx, typename Stream>
        void stream(const Stream*){}

        template <typename Node>
        void treeNode(const Node* node)
        {
            node->processAll(*this);
        }
    };


    bool checkContent(const NodeBaseG& node) const
    {
        if (!Base::checkContent(node))
        {
            if (node->is_leaf())
            {
                CheckContentFn fn(self(), node->id(), node->is_root());

                LeafDispatcher::dispatch(node, fn);

                return fn.errors_;
            }
            else {
                return false;
            }
        }
        else {
            return true;
        }
    }


MEMORIA_V1_CONTAINER_PART_END

}}
