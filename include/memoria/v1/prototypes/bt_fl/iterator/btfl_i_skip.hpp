
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


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorSkipName)

    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using typename Base::Container;

    using typename Base::LeafDispatcher;

    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams                = Container::Types::Streams;
    static const Int DataStreams            = Container::Types::DataStreams;
    static const Int StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:

    CtrSizeT selectFw(CtrSizeT rank, Int stream)
    {
        return self().template select_fw_<IntList<0, 1>>(stream, rank);
    }

    CtrSizeT selectBw(CtrSizeT rank, Int stream)
    {
        return self().template select_fw_<IntList<0, 1>>(stream, rank);
    }


    bool next() {
        return self().skipFw(1) > 0;
    }

    bool prev() {
        return self().skipBw(1) > 0;
    }


    CtrSizeT skipFw(CtrSizeT n)
    {
        return self().template skip_fw_<StructureStreamIdx>(n);
    }


    CtrSizeT skipBw(CtrSizeT n)
    {
        return self().template skip_bw_<StructureStreamIdx>(n);
    }

    CtrSizeT skip(CtrSizeT n)
    {
        if (n > 0) {
            return skipFw(n);
        }
        else {
            return skipBw(n);
        }
    }

    Int stream() const
    {
        auto& self  = this->self();
        auto s      = self.leaf_structure();
        auto idx    = self.idx();

        if (idx < s->size())
        {
            return s->get_symbol(idx);
        }
        else {
            throw Exception(MA_SRC, "End Of Data Structure");
        }
    }

    Int stream_s() const
    {
        auto& self  = this->self();
        auto s      = self.leaf_structure();
        auto idx    = self.idx();

        if (idx < s->size())
        {
            return s->get_symbol(idx);
        }
        else {
            return -1;
        }
    }

    bool isEnd() const
    {
        auto& self = this->self();

        return self.leaf().isSet() ? self.idx() >= self.leaf_size(0) : true;
    }


protected:

    Int data_stream_idx(Int stream) const
    {
        auto& self = this->self();
        return self.data_stream_idx(stream, self.idx());
    }

    Int data_stream_idx(Int stream, Int structure_idx) const
    {
        auto& self = this->self();
        return self.leaf_structure()->rank(structure_idx, stream);
    }


    CtrSizesT leafrank(Int structure_idx) const
    {
        auto& self = this->self();

        auto leaf_structure = self.leaf_structure();

        CtrSizesT ranks;

        for (Int c = 0; c < DataStreams; c++)
        {
            ranks[c] = leaf_structure->rank(structure_idx, c);
        }

        ranks[StructureStreamIdx] = structure_idx;

        return ranks;
    }


    Int structure_size() const
    {
        return self().leaf_size(0);
    }


    const auto* leaf_structure() const
    {
        auto& self = this->self();
        return self.ctr().template getPackedStruct<IntList<StructureStreamIdx, 1>>(self.leaf());
    }

    Int symbol_idx(Int stream, Int position) const
    {
        auto& self = this->self();

        self.leaf_structure()->selectFW(position + 1, stream);
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorSkipName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
