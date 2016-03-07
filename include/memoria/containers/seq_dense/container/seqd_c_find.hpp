
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_FIND_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_FIND_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrFindName)

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

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    CtrSizeT size() {
        return self().sizes()[0];
    }

    CtrSizeT rank(CtrSizeT idx, Int symbol)
    {
        auto& self = this->self();

        typename Types::template RankForwardWalker<Types, IntList<0>> walker(0, symbol, idx);

        auto iter = self.find_(walker);

        return walker.rank();
    }

    CtrSizeT rank(Int symbol) {
        return rank(self().size(), symbol);
    }

    CtrSizeT rank(CtrSizeT start, CtrSizeT idx, Int symbol)
    {
        auto& self = this->self();

        auto iter = self.seek(start);

        return iter->rankFw(idx, symbol);
    }



    auto select(Int symbol, CtrSizeT rank)
    {
        auto& self = this->self();

        MEMORIA_ASSERT(rank, >=, 1);
        MEMORIA_ASSERT(symbol, >=, 0);

        typename Types::template SelectForwardWalker<Types, IntList<0>> walker(symbol, rank);

        return self.find_(walker);
    }

    auto select(CtrSizeT start, Int symbol, CtrSizeT rank)
    {
        auto& self = this->self();

        auto iter = self.seek(start);

        iter->selectFw(symbol, rank);

        return iter;
    }

    Int symbol(Int idx)
    {
        auto& self = this->self();
        return self.seek(idx)->symbol();
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrFindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
