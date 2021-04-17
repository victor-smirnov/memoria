
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(seq_dense::IterSelectName)
public:
    typedef Ctr<typename Types::CtrTypes>                                       Container;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBasePtr                                            NodeBasePtr;

    typedef typename Container::BranchNodeEntry                                 BranchNodeEntry;

    typedef typename Container::Position                                        Position;

    using SymbolsSubstreamPath = typename Container::Types::SymbolsSubstreamPath;

    using CtrSizeT = typename Container::Types::CtrSizeT;


    auto select(CtrSizeT rank_delta, int32_t symbol)
    {
        return self().template ctr_select<SymbolsSubstreamPath>(symbol, rank_delta);
    }

    auto selectFw(CtrSizeT rank_delta, int32_t symbol)
    {
        return self().template iter_select_fw<SymbolsSubstreamPath>(symbol, rank_delta);
    }

    auto selectBw(CtrSizeT rank_delta, int32_t symbol)
    {
        return self().template iter_select_bw<SymbolsSubstreamPath>(symbol, rank_delta);
    }
    
MEMORIA_V1_ITERATOR_PART_END


#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(seq_dense::IterSelectName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS


}
