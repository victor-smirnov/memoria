
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_FIND_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_FIND_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorFindName)

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

    using StreamSizes = typename Container::Types::StreamsSizes;

    template <Int Stream>
    using StreamSizesPath = typename Select<Stream, StreamSizes>::Result;

    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams = Container::Types::Streams;

    using Prefixes = Position[Streams - 1];



private:

    struct FindFn {

    	Position prefix_;
    	Position sums_;
    	Int stream_;
    	CtrSizeT cnt_;

    	FindFn(const Position& prefix, Position& sums, Int stream, Int end):
    		prefix_(prefix), stream_(stream), cnt_(end)
    	{}

    	template <Int Stream, typename Leaf>
    	void process(const Leaf* leaf)
    	{
    		if (Stream >= stream_)
    		{
    			using Path 		 = StreamSizesPath<Stream>;
    			using StreamPath = bttl::BTTLSizePath<Path>;
    			const Int index  = bttl::BTTLSizePathBlockIdx<Path>::Value;

    			auto substream = leaf->template substream<StreamPath>();
    			auto sum = substream->sum(index, prefix_[Stream], prefix_[Stream] + cnt_);

    			sums_[Stream + 1] += sum;
    			cnt_ = sum;
    		}
    	}


    	template <typename NTypes>
    	void treeNode(const LeafNode<NTypes>* leaf)
    	{
    		ForEach<0, Streams - 1>::process(*this, leaf);
    	}
    };


    void count_downstream_items(const Position& prefix, Position& sums, Int stream, Int end) const
    {
    	return LeafDispatcher::dispatch(self().leaf(), CountStreamsItemsFn(prefix, sums, stream, end));
    }



MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorFindName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif

