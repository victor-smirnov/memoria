
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

#include <type_traits>

namespace memoria {
namespace v1 {




MEMORIA_V1_CONTAINER_PART_BEGIN(louds::CtrFindName)
public:
    typedef TypesType                                                           Types;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;
    typedef typename Base::Types::NodeBaseG                                     NodeBaseG;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;

    auto select0(CtrSizeT rank)
    {
        return self().select(0, rank);
    }

    auto select1(CtrSizeT rank)
    {
        return self().select(1, rank);
    }

    CtrSizeT rank1(CtrSizeT idx)
    {
        return self().rank(idx + 1, 1);
    }

    CtrSizeT rank0(CtrSizeT idx)
    {
        return self().rank(idx + 1, 0);
    }

    auto seek(CtrSizeT idx) {
        return self().template seek_stream<0>(idx);
    }


    struct LabelsFn {

        LabelsTuple labels_;


        template <int32_t Idx, typename SeqTypes>
        void stream(const PkdFSSeq<SeqTypes>* seq, int32_t idx)
        {}

        template <int32_t Idx, typename StreamTypes>
        void stream(const PackedFSEArray<StreamTypes>* labels, int32_t idx)
        {
            if (labels)
            {
                std::get<Idx>(labels_) = labels->value(0, idx);
            }
            else {
                std::get<Idx>(labels_) = 0;
            }
        }


        template <int32_t Idx, typename StreamTypes>
        void stream(const PkdVQTree<StreamTypes>* labels, int32_t idx)
        {
            if (labels)
            {
                std::get<Idx>(labels_) = labels->value(0, idx);
            }
            else {
                std::get<Idx>(labels_) = 0;
            }
        }


        template <typename NTypes>
        void treeNode(const LeafNode<NTypes>* node, int32_t label_idx)
        {
            node->template processSubstreams<IntList<1, 1>>(*this, label_idx);
        }
    };

    LabelsTuple getLabels(const NodeBaseG& leaf, int32_t idx) const
    {
        LabelsFn fn;

        LeafDispatcher::dispatch(leaf, fn, idx);

        return fn.labels_;
    }

    LabelsTuple labels(const LoudsNode& node)
    {
        return self().seek(node.node())->labels();
    }

MEMORIA_V1_CONTAINER_PART_END

}}
