
// Copyright 2015 Victor Smirnov
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


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorRemoveName)


    using typename Base::NodeBaseG;
    using typename Base::Container;
    using typename Base::Position;

    using CtrSizeT  = typename Container::Types::CtrSizeT;
    using CtrSizesT = typename Container::Types::CtrSizesT;
    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;

    using BranchNodeEntry               = typename Container::Types::BranchNodeEntry;
    using IteratorBranchNodeEntry       = typename Container::Types::IteratorBranchNodeEntry;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    static const Int Streams                = Container::Types::Streams;
    static const Int SearchableStreams      = Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

public:
    Position remove_subtrees(CtrSizeT n)
    {
        CtrSizesT sizes;

        self().remove_subtrees(n, sizes);

        return sizes;
    }

protected:

    void remove_subtrees(CtrSizeT n, CtrSizesT& sizes)
    {
        MEMORIA_V1_ASSERT(n, >=, 0);

        auto& self  = this->self();

        auto path = self.path();

        auto stream = self.stream();
        auto tmp    = self;

        auto start = self.adjust_to_indel();

        auto start_abs_pos   = self.cache().abs_pos();
        auto start_data_pos  = self.cache().data_pos();
        auto start_data_size = self.cache().data_size();

        tmp.skipFw(n);

        auto end_abs_pos   = tmp.cache().abs_pos();
//      auto end_data_pos  = tmp.cache().data_pos();

        auto length_to_remove = end_abs_pos[stream] - start_abs_pos[stream];

        Position end = tmp.adjust_to_indel();

        self.ctr().removeEntries(self.leaf(), start, tmp.leaf(), end, sizes, true);

        tmp.idx() = end[stream];

        self = tmp;

        self.refresh();

        self.cache().abs_pos()  = start_abs_pos;
        self.cache().data_pos() = start_data_pos;

        self.cache().data_size() = start_data_size;
        self.cache().data_size()[stream] -= length_to_remove;

        if (stream > 0)
        {
            auto tmp2 = self;

            tmp2.toIndex();

            tmp2.add_substream_size(tmp2.stream(), tmp2.idx(), -length_to_remove);
        }

        self = *self.ctr().seek(path, stream).get();
    }


    // Returns leaf ranks of position idx in the specified
    // stream.

    Position local_left_margin() const {
        auto& self = this->self();
        return self.local_left_margin(self.stream(), self.idx());
    }

    Position local_left_margin(Int stream, Int idx) const
    {
        auto& self = this->self();

        Position ranks;

        ranks[stream] = idx;

        for (Int s = stream; s > 0; s--)
        {
            ranks[s - 1] = self.local_parent_idx(s, ranks[s]);
        }

        for (Int s = stream; s < SearchableStreams; s++)
        {
            ranks[s + 1] = self.local_child_idx(s, ranks[s]);
        }

        return ranks;
    }

public:
    Position local_stream_posrank_() const {
        auto& self = this->self();
        return self.local_stream_posrank_(self.stream(), self.idx());
    }

protected:
    Position local_stream_posrank_(Int stream, Int idx) const
    {
        auto& self = this->self();

        Position ranks;

//      auto sizes = self.leaf_sizes();

//      auto extent = self.leaf_extent();

        ranks[stream] = idx;

        Int parent_idx = ranks[stream];

        for (Int s = stream; s > 0; s--)
        {
            parent_idx = self.local_parent_idx(s, parent_idx);

            if (parent_idx >= 0) {
                ranks[s - 1] = parent_idx + 1;//(sizes[s - 1] > 0 ? 1 : 0);
            }
            else {
                break;
            }
        }

        for (Int s = stream; s < SearchableStreams; s++)
        {
            ranks[s + 1] = self.local_child_idx(s, ranks[s]);
        }

        return ranks;
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorRemoveName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
