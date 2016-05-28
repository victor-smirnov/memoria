
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorFindName)


    using Container = typename Base::Container;
    using CtrSizeT  = typename Container::Types::CtrSizeT;

    static const Int Streams                = Container::Types::Streams;
    static const Int DataStreams            = Container::Types::DataStreams;
    static const Int StructureStreamIdx     = Container::Types::StructureStreamIdx;



public:
    auto countFw()
    {
        typename Types::template CountForwardWalker<Types, IntList<StructureStreamIdx, 1>> walker;
        return self().find_fw(walker);
    }

    auto countBw()
    {
        typename Types::template CountBackwardWalker<Types, IntList<StructureStreamIdx, 1>> walker;
        return self().find_bw(walker);
    }



    CtrSizeT selectFw(CtrSizeT rank, Int stream)
    {
        return self().template select_fw_<IntList<StructureStreamIdx, 1>>(stream, rank);
    }

    CtrSizeT selectBw(CtrSizeT rank, Int stream)
    {
        return self().template select_fw_<IntList<StructureStreamIdx, 1>>(stream, rank);
    }



    using Base::finish_walking;

    template <typename Walker>
    void do_finish_walking(Int idx, const Walker& walker, WalkCmd cmd) {
        if (cmd != WalkCmd::REFRESH)
        {
            constexpr Int stream = ListHead<typename Walker::LeafPath>::Value;
            auto& self = this->self();
            self.idx() = self.symbol_idx(stream, idx);
        }
    }


    template <typename WWTypes>
    void finish_walking(Int idx, const FindForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }

    template <typename WWTypes>
    void finish_walking(Int idx, const FindBackwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }


    template <typename WWTypes>
    void finish_walking(Int idx, const FindGEForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }

    template <typename WWTypes>
    void finish_walking(Int idx, const FindGEBackwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }

    template <typename WWTypes>
    void finish_walking(Int idx, const FindMaxGEWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }



MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorFindName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
