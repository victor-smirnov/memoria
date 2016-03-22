
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorInsertName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::BranchNodeEntry                               BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT  = typename Container::Types::CtrSizeT;
    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;
    using IteratorBranchNodeEntry       = typename Container::Types::IteratorBranchNodeEntry;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams                = Container::Types::Streams;
    static const Int SearchableStreams      = Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;


    SplitStatus split()
    {
        auto& self  = this->self();
        auto& leaf  = self.leaf();
        auto stream = self.stream();

        auto sizes = self.leaf_sizes();
        auto full_leaf_size = sizes.sum();

        if (full_leaf_size > 1)
        {
            auto half_ranks = self.leafrank_(full_leaf_size/2);
            auto right = self.ctr().split_leaf_p(leaf, half_ranks);

            auto& idx = self.idx();

            if (idx > half_ranks[stream])
            {
                leaf = right;
                idx -= half_ranks[stream];

                self.refresh();

                return SplitStatus::RIGHT;
            }
            else {
                return SplitStatus::LEFT;
            }
        }
        else {
            self.ctr().split_leaf_p(leaf, sizes);
            return SplitStatus::LEFT;
        }
    }





    template <Int StreamIdx, typename EntryBuffer>
    SplitStatus _insert(const EntryBuffer& data)
    {
        auto& self  = this->self();
        auto stream = self.stream();

        MEMORIA_ASSERT(StreamIdx, ==, stream);

        auto main_split_status = self.ctr().template insert_stream_entry<StreamIdx>(self, data);

        auto tmp = self;
        tmp.toIndex();

        if (tmp.has_same_leaf(self))
        {
            auto status = tmp.add_substream_size(tmp.stream(), tmp.idx(), 1);

            if (status == SplitStatus::NONE)
            {
                return main_split_status;
            }
            else
            {
                auto pos = self.pos();

                self = tmp;
                self.toData(pos);

                return SplitStatus::UNKNOWN;
            }
        }
        else {
            tmp.add_substream_size(tmp.stream(), tmp.idx(), 1);
        }

        return main_split_status;
    }





MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorInsertName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS
