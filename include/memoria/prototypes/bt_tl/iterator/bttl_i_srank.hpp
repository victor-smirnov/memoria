
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_SRANK_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_SRANK_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorStreamRankName)

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

    using Ranks = Position[Streams - 1];

    Position leaf_rank(const Position& sizes, const Ranks& prefixes, Int pos) const
    {
    	for (Int s = 0; s < Streams; s++)
    	{
    		Int sum = prefixes[s].sum();
    		if (pos >= sum)
    		{
    			return bttl::detail::ZeroRankHelper<0, Streams>::process(this, sizes, prefixes[s], pos);
    		}
    	}

    	return Position();
    }

    Position leaf_rank(Int pos) const
    {
    	const auto& ranks = self().cache().ranks();
//    	compute_leaf_prefixes(prefixes);

    	return leaf_rank(self().leaf_sizes(), ranks, pos);
    }



    void compute_leaf_prefixes(Ranks& prefixes) const
    {
    	const auto& self  = this->self();
    	const auto& cache = self.cache();

    	const auto& branch_prefix = cache.prefixes();

    	Position expected_sizes;
    	bttl::detail::ExpectedSizesHelper<StreamSizes, AccumItemH>::process(branch_prefix, expected_sizes);

    	const auto& actual_sizes = cache.size_prefix();

    	expected_sizes[0] = actual_sizes[0];

    	auto extent = expected_sizes - actual_sizes;

    	prefixes[Streams - 2][Streams - 1] = extent[Streams - 1];

    	for (Int s = Streams - 2; s > 0; s--)
    	{
    		prefixes[s] = prefixes[s + 1];
    		count_downstream_items(prefixes[s + 1], prefixes[s], s, extent[s]);
    	}
    }

    template <Int Stream>
    struct _StreamsRankFn {
    	template <typename NTypes, typename... Args>
    	Position treeNode(const LeafNode<NTypes>* leaf, const Position& sizes, const Position& prefix, Int pos, Args&&... args)
    	{
    		bttl::detail::StreamsRankFn<
				StreamSizesPath,
				CtrSizeT,
				Position
			> fn(sizes, prefix, pos);

    		bttl::detail::StreamsRankHelper<Stream, Streams - 1>::process(leaf, fn, std::forward<Args>(args)...);

    		return fn.indexes_;
    	}
    };

    template <Int Stream, typename... Args>
    Position _streams_rank(Args&&... args) const
    {
    	return LeafDispatcher::dispatch(self().leaf(), _StreamsRankFn<Stream>(), std::forward<Args>(args)...);
    }

private:

    struct CountStreamsItemsFn {

    	Position prefix_;
    	Position sums_;
    	Int stream_;
    	CtrSizeT cnt_;

    	CountStreamsItemsFn(const Position& prefix, Position& sums, Int stream, Int end):
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

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorStreamRankName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif

