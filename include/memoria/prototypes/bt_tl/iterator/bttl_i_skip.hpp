
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_SKIP_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_SKIP_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorSkipName)

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

    static const Int Streams 				= Container::Types::Streams;
    static const Int SearchableStreams 		= Container::Types::SearchableStreams;


    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

    struct SkipFwFn {

    	CtrSizeT distance_ = 0;

    	template <Int Stream, typename Itr>
    	bool process(Itr&& iter, Int stream, CtrSizeT n)
    	{
    		if (Stream == stream)
    		{
    			distance_ = iter.template _skipFw<Stream>(n);
    			return false;
    		}
    		else {
    			return true;
    		}
    	}
    };

    struct SkipBwFn {

    	CtrSizeT distance_ = 0;

    	template <Int Stream, typename Itr>
    	bool process(Itr&& iter, Int stream, CtrSizeT n)
    	{
    		if (Stream == stream)
    		{
    			distance_ = iter.template _skipBw<Stream>(n);
    			return false;
    		}
    		else {
    			return true;
    		}
    	}
    };


    CtrSizeT skipFw(CtrSizeT n)
    {
    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto stream = self.stream();

    	auto pos  = cache.data_pos()[stream];
    	auto size = cache.data_size()[stream];

    	if (pos + n > size) {
    		//n = size - pos;
    	}

    	SkipFwFn fn;
    	ForEach<0, Streams>::process(fn, self, stream, n);
    	return fn.distance_;
    }


    CtrSizeT skipBw(CtrSizeT n)
    {
    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto stream = self.stream();

    	auto pos  = cache.data_pos()[stream];

    	if (pos - n < 0) {
    		n = pos;
    	}

    	SkipBwFn fn;
    	ForEach<0, Streams>::process(fn, self, stream, n);
    	return fn.distance_;
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

    CtrSizeT size() const {
    	auto stream = self().stream();
    	return self().cache().data_size(stream);
    }


    CtrSizeT pos() const {
    	auto stream = self().stream();
    	return self().cache().data_pos(stream);
    }

    CtrSizeT toData(CtrSizeT offset = 0)
    {
    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& stream = self.stream();

    	auto& idx = self.idx();

    	if (stream < SearchableStreams)
    	{
    		auto substream_offset 	= self.count_items(stream, idx);
    		auto stream_prefix 		= self.data_offset(stream + 1);

    		auto full_substream_offset = stream_prefix + substream_offset;

    		cache.data_pos()[stream + 1] = -full_substream_offset;

    		stream++;
    		idx = 0;

    		return self.skipFw(full_substream_offset + offset);
    	}
    	else {
    		return 0;
    	}
    }

    void toIndex()
    {
    	auto& self = this->self();

    	auto& stream = self.stream();

    	auto& idx = self.idx();

    	if (stream > 0)
    	{
    		auto stream_prefix = self.data_offset(stream - 1);
    		if (idx >= stream_prefix)
    		{
    			auto parent_idx = self.find_offset(stream, idx);

    			stream--;
    			idx = 0;

    			self.skipFw(parent_idx);
    		}
    		else {
    			stream--;
    			idx = 0;

    			self.skipBw(1);
    		}
    	}
    }




    struct FindOffsetFn {

    	Int stream_;
    	Int target_;
    	Int idx_ = -2;

    	FindOffsetFn(Int stream, Int target):
    		stream_(stream), target_(target)
    	{}

    	template <Int Stream, typename Leaf>
    	bool process(const Leaf* leaf)
    	{
    		if (Stream == stream_)
    		{
    			using Path 		 = StreamSizesPath<Stream>;
    			using StreamPath = bttl::BTTLSizePath<Path>;
    			const Int index  = bttl::BTTLSizePathBlockIdx<Path>::Value;

    			auto substream = leaf->template substream<StreamPath>();
    			auto result = substream->findGTForward(index, 0, target_);

    			idx_ = result.idx();

    			return false;
    		}

    		return true;
    	}


    	template <typename NTypes>
    	Int treeNode(const LeafNode<NTypes>* leaf)
    	{
    		ForEach<0, SearchableStreams>::process(*this, leaf);
    		return idx_;
    	}
    };


    Int find_offset(Int stream, Int idx) const
    {
    	MEMORIA_ASSERT_TRUE(stream >= 0 && stream < SearchableStreams);
    	MEMORIA_ASSERT(idx, >=, 0);

    	return LeafDispatcher::dispatch(self().leaf(), FindOffsetFn(stream, idx));
    }



    struct DataOffsetFn {
    	CtrSizeT expected_size_ = -1;

    	template <Int Stream, typename Cache>
    	bool process(Cache&& cache, Int stream)
    	{
    		using Path		 = StreamSizesPath<Stream>;
    		using StreamPath = bttl::BTTLSizePath<Path>;
    		const Int index  = bttl::BTTLSizePathBlockIdx<Path>::Value;

    		if (Stream == stream)
    		{
    			expected_size_ = AccumItemH<StreamPath>::value(index, cache.prefixes());
    			return false;
    		}
    		else {
    			return true;
    		}
    	}
    };


    CtrSizeT data_offset(Int stream) const
    {
    	auto& self 	= this->self();
    	auto& cache	= self.cache();

    	if (stream > 0)
    	{
    		DataOffsetFn fn;

    		ForEach<0, SearchableStreams>::process(fn, cache, stream - 1);

    		MEMORIA_ASSERT(fn.expected_size_, >=, 0);

    		auto actual_size = cache.size_prefix()[stream];

    		return fn.expected_size_ - actual_size;
    	}
    	else {
    		return 0;
    	}
    }

    template <Int Stream>
    struct IdxDataSizeFn {
    	template <typename NTypes, typename... Args>
    	auto treeNode(const LeafNode<NTypes>* node, Args&&... args)
    	{
    		using Path = StreamSizesPath<Stream>;

			using StreamPath = bttl::BTTLSizePath<Path>;
			const Int index  = bttl::BTTLSizePathBlockIdx<Path>::Value;

    		return node->template substream<StreamPath>()->value(index, std::forward<Args>(args)...);
    	}
    };

    template <Int Stream>
    auto idx_data_size(Int idx) const
    {
    	auto& self = this->self();
    	return LeafDispatcher::dispatch(self.leaf(), IdxDataSizeFn<Stream>(), idx);
    }

    struct CountStreamItemsFn {

    	Int stream_;
    	CtrSizeT sum_ = 0;
    	Int end_;

    	CountStreamItemsFn(Int stream, Int end):
    		stream_(stream), end_(end)
    	{}

    	template <Int Stream, typename Leaf>
    	bool process(const Leaf* leaf)
    	{
    		if (Stream == stream_)
    		{
    			using Path 		 = StreamSizesPath<Stream>;
    			using StreamPath = bttl::BTTLSizePath<Path>;
    			const Int index  = bttl::BTTLSizePathBlockIdx<Path>::Value;

    			auto substream = leaf->template substream<StreamPath>();
    			sum_ = substream->sum(index, end_);

    			return false;
    		}

    		return true;
    	}


    	template <typename NTypes>
    	CtrSizeT treeNode(const LeafNode<NTypes>* leaf)
    	{
    		ForEach<0, SearchableStreams>::process(*this, leaf);
    		return sum_;
    	}
    };


    CtrSizeT count_items(Int stream, Int end) const
    {
    	return LeafDispatcher::dispatch(self().leaf(), CountStreamItemsFn(stream, end));
    }




    template <typename Walker>
    void finish_walking(Int idx, Walker& w, WalkCmd cmd) {
    	Base::finish_walking(idx, w, cmd);
    }




    template <typename WTypes>
    void finish_walking(Int idx, const SkipForwardWalker<WTypes>& walker, WalkCmd cmd)
    {
    	constexpr Int Stream = SkipForwardWalker<WTypes>::Stream;

    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& pos  = cache.data_pos();
    	auto& size = cache.data_size();

    	auto stream = self.stream();

    	pos[stream] += walker.sum();

    	self.template _update_substream_size<Stream>(stream, idx, size);


    	self.update_leaf_ranks(cmd);
    }

    template <typename WTypes>
    void finish_walking(Int idx, const SkipBackwardWalker<WTypes>& walker, WalkCmd cmd)
    {
    	constexpr Int Stream = SkipBackwardWalker<WTypes>::Stream;

    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& pos  = cache.data_pos();
    	auto& size = cache.data_size();

    	auto stream = self.stream();

    	pos[stream] -= walker.sum();

    	self.template _update_substream_size<Stream>(stream, idx, size);

    	self.update_leaf_ranks(cmd);
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorSkipName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif

