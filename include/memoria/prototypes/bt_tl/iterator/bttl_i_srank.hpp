
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

    using CtrSizeT 				= typename Container::Types::CtrSizeT;
    using IteratorAccumulator 	= typename Container::Types::IteratorAccumulator;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>;


    template <Int StreamIdx>
    using LeafSizesSubstreamPath = typename Container::Types::template LeafSizesSubstreamPath<StreamIdx>;


    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams 				= Container::Types::Streams;
    static const Int SearchableStreams 		= Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

    Position leaf_rank(const Position& sizes, const LeafPrefixRanks& prefixes, Int pos) const
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
    	LeafPrefixRanks ranks;
    	compute_leaf_prefixes(ranks);

    	return leaf_rank(self().leaf_sizes(), ranks, pos);
    }


    Position leaf_extent() const
    {
    	const auto& self  = this->self();
    	const auto& cache = self.cache();
    	const auto& branch_prefix = cache.prefixes();

    	Position expected_sizes;
    	bttl::detail::ExpectedSizesHelper<Streams - 1, LeafSizesSubstreamPath, AccumItemH>::process(branch_prefix, expected_sizes);

    	expected_sizes[0] = cache.size_prefix()[0];

    	return expected_sizes - cache.size_prefix();
    }


    template <Int Stream>
    struct _StreamsRankFn {
    	template <typename NTypes, typename... Args>
    	Position treeNode(const LeafNode<NTypes>* leaf, const Position& sizes, const Position& prefix, Int pos, Args&&... args)
    	{
    		bttl::detail::StreamsRankFn<
				LeafSizesSubstreamPath,
				CtrSizeT,
				Position
			> fn(sizes, prefix, pos);

    		bttl::detail::StreamsRankHelper<Stream, SearchableStreams>::process(leaf, fn, std::forward<Args>(args)...);

    		return fn.indexes_;
    	}
    };

    template <Int Stream, typename... Args>
    Position _streams_rank(Args&&... args) const
    {
    	return LeafDispatcher::dispatch(self().leaf(), _StreamsRankFn<Stream>(), std::forward<Args>(args)...);
    }

    void dumpRanks(std::ostream& out = std::cout) const
    {
    	LeafPrefixRanks ranks;

    	self().compute_leaf_prefixes(ranks);

    	out<<"PrefixRanks:"<<endl;

    	for (Int c = 0; c < Streams; c++) {
    		out<<ranks[c]<<endl;
    	}
    }

    void dumpExtent(std::ostream& out = std::cout) const
    {
    	out<<"LeafExtent:"<<self().leaf_extent()<<endl;
    }


    void compute_leaf_prefixes(LeafPrefixRanks& prefixes) const
    {
    	const auto& self  = this->self();
    	const auto& cache = self.cache();

    	const auto& branch_prefix = cache.prefixes();

    	Position expected_sizes;
    	bttl::detail::ExpectedSizesHelper<Streams - 1, LeafSizesSubstreamPath, AccumItemH>::process(branch_prefix, expected_sizes);

    	const auto& actual_sizes = cache.size_prefix();

    	expected_sizes[0] = actual_sizes[0];

    	auto extent = expected_sizes - actual_sizes;

    	prefixes[Streams - 2][Streams - 1] = extent[Streams - 1];

    	for (Int s = SearchableStreams - 1; s > 0; s--)
    	{
    		prefixes[s - 1] = prefixes[s];
    		count_downstream_items(prefixes[s], prefixes[s - 1], s, extent[s]);
    		prefixes[s - 1][s] = extent[s];
    	}
    }



private:

    struct CountStreamsItemsFn {

    	Position prefix_;
    	Position& sums_;
    	Int stream_;
    	CtrSizeT cnt_;

    	CountStreamsItemsFn(const Position& prefix, Position& sums, Int stream, Int end):
    		prefix_(prefix), sums_(sums), stream_(stream), cnt_(end)
    	{}

    	template <typename Stream>
    	auto stream(const Stream* substream, CtrSizeT start, CtrSizeT end) {
    		return substream->sum(0, start, end);
    	}


    	template <Int StreamIdx, typename Leaf>
    	bool process(const Leaf* leaf)
    	{
    		if (StreamIdx >= stream_)
    		{
    			constexpr Int SubstreamIdx = Leaf::template StreamStartIdx<StreamIdx>::Value +
    			    						 Leaf::template StreamSize<StreamIdx>::Value - 1;

    			auto sum = Leaf::Dispatcher::template dispatch<SubstreamIdx>(
    					leaf->allocator(),
						*this,
						prefix_[StreamIdx],
						prefix_[StreamIdx] + cnt_
				);

    			sums_[StreamIdx + 1] += sum;
    			cnt_ = sum;
    		}

    		return true;
    	}


    	template <typename NTypes>
    	void treeNode(const LeafNode<NTypes>* leaf)
    	{
    		ForEach<0, SearchableStreams>::process(*this, leaf);
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

