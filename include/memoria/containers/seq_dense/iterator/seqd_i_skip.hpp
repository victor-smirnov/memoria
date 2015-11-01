
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_SKIP_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_SKIP_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterSkipName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    bool operator++() {
        return self().skipFw(1);
    }

    bool operator--() {
        return self().skipBw(1);
    }

    bool operator++(int) {
        return self().skipFw(1);
    }

    bool operator--(int) {
        return self().skipBw(1);
    }

    BigInt operator+=(BigInt size)
    {
        return self().skipFw(size);
    }

    BigInt operator-=(BigInt size)
    {
        return self().skipBw(size);
    }

    bool isEof() const {
        return self().idx() >= self().size();
    }

    bool isBof() const {
        return self().idx() < 0;
    }

    Int size() const
    {
        return self().leafSize(0);
    }


    auto skipFw(CtrSizeT amount) {
    	return self().template _skipFw<0>(amount);
    }

    auto skipBw(CtrSizeT amount) {
    	return self().template _skipBw<0>(amount);
    }

    auto skip(CtrSizeT amount) {
    	return self().template _skip<0>(amount);
    }


    struct PosFn {
        BigInt prefix_ = 0;

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, Int idx) {}

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, Int idx)
        {
            node->sum(0, 0, 0, idx, prefix_);
        }
    };


    BigInt pos() const
    {
        auto& self = this->self();

        PosFn fn;

        self.ctr().walkUp(self.leaf(), self.idx(), fn);

        return fn.prefix_ + self.idx();
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterSkipName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS


}



#endif
