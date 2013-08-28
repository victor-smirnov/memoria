
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_RANK_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_RANK_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterRankName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    struct RankFn {
        BigInt rank_ = 0;
        Int symbol_;
        RankFn(Int symbol): symbol_(symbol) {}

        template <Int Idx, typename StreamTypes>
        void stream(const PkdFSSeq<StreamTypes>* seq, Int idx)
        {
            if (seq != nullptr) {
                rank_ += seq->rank(0, idx, symbol_);
            }
        }

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, Int idx)
        {
            node->template processStream<0>(*this, idx);
        }

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, Int idx)
        {
            if (node != nullptr)
            {
                node->sum(0, symbol_ + 1, 0, idx, rank_);
            }
        }
    };

    BigInt rank(Int symbol) const;
    BigInt ranki(Int symbol) const;

    BigInt local_rank(Int idx, Int symbol) const;

    BigInt rank(BigInt delta, Int symbol);
    BigInt rankFw(BigInt delta, Int symbol);
    BigInt rankBw(BigInt delta, Int symbol);

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterRankName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::rank(Int symbol) const
{
    auto& self = this->self();

    RankFn fn(symbol);

    if (self.idx() >= 0)
    {
        self.ctr().walkUp(self.leaf(), self.idx(), fn);
    }

    return fn.rank_;
}

M_PARAMS
BigInt M_TYPE::local_rank(Int idx, Int symbol) const
{
    auto& self = this->self();

    RankFn fn(symbol);

    LeafDispatcher::dispatchConst(self.leaf(), fn, idx);

    return fn.rank_;
}


M_PARAMS
BigInt M_TYPE::ranki(Int symbol) const
{
    auto& self = this->self();

    RankFn fn(symbol);

    self.ctr().walkUp(self.leaf(), self.idx() + 1, fn);

    return fn.rank_;
}

M_PARAMS
BigInt M_TYPE::rank(BigInt delta, Int symbol)
{
    auto& self  = this->self();

    if (delta > 0)
    {
        return self.rankFw(delta, symbol);
    }
    else if (delta < 0)
    {
        return self.rankBw(-delta, symbol);
    }
    else {
        return 0;
    }
}

M_PARAMS
BigInt M_TYPE::rankFw(BigInt delta, Int symbol)
{
    auto& self  = this->self();
    auto& ctr   = self.ctr();
    Int stream  = self.stream();

    if (delta < 0) {
        int a = 0; a++;
    }

    MEMORIA_ASSERT(delta, >=, 0);

    typename Types::template RankFWWalker<Types> walker(stream, symbol, delta);

    walker.prepare(self);

    Int idx = ctr.findFw(self.leaf(), stream, self.idx(), walker);

    return walker.finish(self, idx);
}

M_PARAMS
BigInt M_TYPE::rankBw(BigInt delta, Int symbol)
{
    auto& self  = this->self();
    auto& ctr   = self.ctr();
    Int stream  = self.stream();

    MEMORIA_ASSERT(delta, >=, 0);

    typename Types::template RankBWWalker<Types> walker(stream, symbol, delta);

    walker.prepare(self);

    Int idx = ctr.findBw(self.leaf(), stream, self.idx(), walker);

    return walker.finish(self, idx);
}




#undef M_TYPE
#undef M_PARAMS


}



#endif
