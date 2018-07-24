
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memoria/v1/containers/seq_dense/seqd_names.hpp>
#include <memoria/v1/containers/seq_dense/seqd_tools.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt/bt_macros.hpp>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(seq_dense::IterRankName)
public:
    typedef Ctr<typename Types::CtrTypes>                                       Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;

    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    using SymbolsSubstreamPath = typename Container::Types::SymbolsSubstreamPath;

    template <typename RankSubstreamPath>
    struct RankFn {
        int64_t rank_ = 0;
        int32_t symbol_;
        RankFn(int32_t symbol): symbol_(symbol) {}

        template <int32_t Idx, typename StreamTypes>
        void stream(const PkdFSSeq<StreamTypes>* seq, int32_t idx)
        {
            if (seq != nullptr) {
                rank_ += seq->rank(0, idx, symbol_);
            }
        }

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, WalkCmd, int32_t start, int32_t idx)
        {
            node->template processStream<RankSubstreamPath>(*this, idx);
        }

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, int32_t idx)
        {
            node->template processStream<RankSubstreamPath>(*this, idx);
        }

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, WalkCmd, int32_t start, int32_t idx)
        {
            if (node != nullptr)
            {
                node->template sum_substream_for_leaf_path<RankSubstreamPath>(symbol_, 0, idx, rank_);
            }
        }
    };

    int64_t rank(int32_t symbol) const;
    int64_t ranki(int32_t symbol) const;

    int64_t localrank_(int32_t idx, int32_t symbol) const;

    auto rank(int64_t delta, int32_t symbol)
    {
        return self().template rank_<SymbolsSubstreamPath>(symbol, delta);
    }

    auto rankFw(int64_t delta, int32_t symbol)
    {
        return self().template rank_fw_<SymbolsSubstreamPath>(symbol, delta);
    }

    auto rankBw(int64_t delta, int32_t symbol)
    {
        return self().template rank_bw_<SymbolsSubstreamPath>(symbol, delta);
    }

MEMORIA_V1_ITERATOR_PART_END


#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(seq_dense::IterRankName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
int64_t M_TYPE::rank(int32_t symbol) const
{
    auto& self = this->self();

    RankFn<IntList<0, 1>> fn(symbol);

    if (self.idx() >= 0)
    {
        self.ctr().walkUp(self.leaf(), self.idx(), fn);
    }

    return fn.rank_;
}

M_PARAMS
int64_t M_TYPE::localrank_(int32_t idx, int32_t symbol) const
{
    auto& self = this->self();

    RankFn<IntList<0, 1>> fn(symbol);

    LeafDispatcher::dispatch(self.leaf(), fn, idx);

    return fn.rank_;
}


M_PARAMS
int64_t M_TYPE::ranki(int32_t symbol) const
{
    auto& self = this->self();

    RankFn<IntList<0, 1>> fn(symbol);

    self.ctr().walkUp(self.leaf(), self.idx() + 1, fn);

    return fn.rank_;
}

//M_PARAMS
//int64_t M_TYPE::rank(int64_t delta, int32_t symbol)
//{
//    auto& self  = this->self();
//
//    if (delta > 0)
//    {
//        return self.rankFw(delta, symbol);
//    }
//    else if (delta < 0)
//    {
//        return self.rankBw(-delta, symbol);
//    }
//    else {
//        return 0;
//    }
//}
//
//M_PARAMS
//int64_t M_TYPE::rankFw(int64_t delta, int32_t symbol)
//{
//    auto& self  = this->self();
//
//    MEMORIA_V1_ASSERT(delta, >=, 0);
//
//    typename Types::template RankFWWalker<Types, IntList<0>> walker(symbol, delta);
//
//
//    return self.find_fw(walker);
//}
//
//M_PARAMS
//int64_t M_TYPE::rankBw(int64_t delta, int32_t symbol)
//{
//    auto& self  = this->self();
//
//    MEMORIA_V1_ASSERT(delta, >=, 0);
//
//    typename Types::template RankBWWalker<Types, IntList<0>> walker(symbol, delta);
//
//    return self.template find_bw(walker);
//}




#undef M_TYPE
#undef M_PARAMS


}}
