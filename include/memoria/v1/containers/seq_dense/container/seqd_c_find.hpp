
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
public:
    using Types = typename Base::Types;
    using typename Base::CtrSizeT;

    using SymbolsSubstreamPath = typename Types::SymbolsSubstreamPath;


    CtrSizeT size() {
        return self().sizes()[0];
    }

    CtrSizeT rank(CtrSizeT idx, int32_t symbol)
    {
        auto& self = this->self();

        typename Types::template RankForwardWalker<Types, SymbolsSubstreamPath> walker(symbol, idx);

        self.find_(walker);

        return walker.rank();
    }

    CtrSizeT rank(int32_t symbol) {
        return rank(self().size(), symbol);
    }

    CtrSizeT rank(CtrSizeT start, CtrSizeT idx, int32_t symbol)
    {
        auto& self = this->self();

        auto iter = self.seek(start);

        return iter->rankFw(idx, symbol);
    }



    auto select(int32_t symbol, CtrSizeT rank)
    {
        auto& self = this->self();

        MEMORIA_V1_ASSERT(rank, >=, 1);
        MEMORIA_V1_ASSERT(symbol, >=, 0);

        typename Types::template SelectForwardWalker<Types, SymbolsSubstreamPath> walker(symbol, rank);

        return self.find_(walker);
    }

    auto select(CtrSizeT start, int32_t symbol, CtrSizeT rank)
    {
        auto& self = this->self();

        auto iter = self.seek(start);

        iter->selectFw(symbol, rank);

        return iter;
    }

    int32_t symbol(int32_t idx)
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
