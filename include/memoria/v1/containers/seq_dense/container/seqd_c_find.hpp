
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

#include <memoria/v1/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/v1/containers/seq_dense/seqd_names.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::seq_dense::CtrFindName)

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

        MEMORIA_V1_ASSERT(rank, >=, 1);
        MEMORIA_V1_ASSERT(symbol, >=, 0);

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



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::seq_dense::CtrFindName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}