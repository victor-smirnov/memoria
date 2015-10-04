
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_REMOVE_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_REMOVE_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorRemoveName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT 	= typename Container::Types::CtrSizeT;
    using Key		= typename Container::Types::Key;
    using Value		= typename Container::Types::Value;
    using IteratorAccumulator		= typename Container::Types::IteratorAccumulator;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>;



    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams 				= Container::Types::Streams;
    static const Int SearchableStreams 		= Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

    template <Int Stream>
    using StreamInputTuple = typename Container::Types::template StreamInputTuple<Stream>;

    void remove_subtrees(CtrSizeT n) {
    	Accumulator sums;

    	self().remove_subtrees(n, sums);

    	cout<<"Sums: "<<sums<<endl;
    }

    void remove_subtrees(CtrSizeT n, Accumulator& sums)
    {
    	auto& self 	= this->self();
    	auto stream = self.stream();
    	auto tmp 	= self;

    	auto start_abs_pos 	 = self.cache().abs_pos();
    	auto start_data_pos  = self.cache().data_pos();
    	auto start_data_size = self.cache().data_size();

    	Position start = self.local_left_margin(stream, self.idx());

    	tmp.skipFw(n);

    	auto end_abs_pos   = tmp.cache().abs_pos();
    	auto end_data_pos  = tmp.cache().data_pos();

    	auto length_to_remove = end_abs_pos[stream] - start_abs_pos[stream];

    	Position end = tmp.local_left_margin(stream, tmp.idx());

    	self.ctr().removeEntries(self.leaf(), start, tmp.leaf(), end, sums, true);

    	tmp.idx() = end[stream];

    	self = tmp;

    	self.refresh();

    	self.cache().abs_pos() = start_abs_pos;
    	self.cache().data_pos() = start_data_pos;

    	self.cache().data_size() = start_data_size;
    	self.cache().data_size()[stream] -= length_to_remove;

    	if (stream > 0)
    	{
    		auto tmp2 = self;
    		tmp2.toIndex();

    		tmp2.dump();

    		tmp2.add_substream_size(tmp2.stream(), tmp2.idx(), -length_to_remove);
    	}
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

    	for (Int s = stream - 1; s > 0; s--)
    	{
    		ranks[s] = self.local_parent_idx(s, ranks[s + 1]);
    	}

    	for (Int s = stream; s < SearchableStreams; s++)
    	{
    		ranks[s + 1] = self.local_child_idx(s, ranks[s]);
    	}

    	return ranks;
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorRemoveName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
