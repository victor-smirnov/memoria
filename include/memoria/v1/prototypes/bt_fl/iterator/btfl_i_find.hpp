
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorFindName)


    using Container = typename Base::Container;
    using CtrSizeT  = typename Container::Types::CtrSizeT;

    static const int32_t Streams                = Container::Types::Streams;
    static const int32_t DataStreams            = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;



public:
    auto iter_count_fw()
    {
        typename Types::template CountForwardWalker<Types, IntList<StructureStreamIdx, 1>> walker;
        return self().iter_find_fw(walker);
    }

    auto iter_count_bw()
    {
        typename Types::template CountBackwardWalker<Types, IntList<StructureStreamIdx, 1>> walker;
        return self().iter_find_bw(walker);
    }



    CtrSizeT iter_btfl_select_fw(CtrSizeT rank, int32_t stream)
    {
        return self().template iter_select_fw<IntList<StructureStreamIdx, 1>>(stream, rank);
    }

    CtrSizeT iter_select_ge_fw(CtrSizeT rank, int32_t stream)
    {
    	typename Types::template SelectGEForwardWalker<Types, IntList<StructureStreamIdx, 1>> walker(stream, rank);
    	return self().iter_find_fw(walker);
    }

    CtrSizeT iter_select_pos_fw(CtrSizeT rank, int32_t stream) const
    {
        const auto& self = this->self();

        auto ii = self.iter_clone();
        ii->selectFw(rank, stream);
        return ii->pos();
    }

    CtrSizeT iter_btfl_select_bw(CtrSizeT rank, int32_t stream)
    {
        return self().template iter_select_bw<IntList<StructureStreamIdx, 1>>(stream, rank);
    }



    using Base::iter_finish_walking;

    template <typename Walker>
    void do_finish_walking(int32_t idx, const Walker& walker, WalkCmd cmd) {
        if (cmd != WalkCmd::REFRESH)
        {
            constexpr int32_t stream = ListHead<typename Walker::LeafPath>::Value;
            auto& self = this->self();
            int iidx = self.symbol_idx(stream, idx);
            self.iter_local_pos() = iidx;
        }
    }


    template <typename WWTypes>
    void iter_finish_walking(int32_t idx, const bt::FindForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }

    template <typename WWTypes>
    void iter_finish_walking(int32_t idx, const bt::FindBackwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }


    template <typename WWTypes>
    void iter_finish_walking(int32_t idx, const bt::FindGEForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }

    template <typename WWTypes>
    void iter_finish_walking(int32_t idx, const bt::FindGEBackwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }

    template <typename WWTypes>
    void iter_finish_walking(int32_t idx, const bt::FindMaxGEWalker<WWTypes>& walker, WalkCmd cmd)
    {
        do_finish_walking(idx, walker, cmd);
    }



MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorFindName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
