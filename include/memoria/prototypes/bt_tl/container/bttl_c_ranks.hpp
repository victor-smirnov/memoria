
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTTL_CTR_RANKS_HPP
#define _MEMORIA_PROTOTYPES_BTTL_CTR_RANKS_HPP


#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bttl::RanksName)

    using Types 			= typename Base::Types;

    using NodeBaseG 		= typename Types::NodeBaseG;
    using Iterator  		= typename Base::Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    using Key 				    = typename Types::Key;
    using Value 			    = typename Types::Value;
    using CtrSizeT			    = typename Types::CtrSizeT;

    using Accumulator 		    = typename Types::Accumulator;
    using Position 			    = typename Types::Position;

    static const Int Streams 			= Types::Streams;
    static const Int SearchableStreams 	= Types::SearchableStreams;

    using PageUpdateMgt 	= typename Types::PageUpdateMgr;

    using LeafPrefixRanks 	= typename Types::LeafPrefixRanks;

    template <Int StreamIdx>
    using LeafSizesSubstreamPath = typename Types::template LeafSizesSubstreamPath<StreamIdx>;


    template <Int StreamIdx>
    struct ProcessCountSubstreamFn {
    	template <typename Node, typename Fn, typename... Args>
    	auto treeNode(Node* node, Fn&& fn, Args&&... args)
    	{
    		constexpr Int SubstreamIdx = Node::template StreamStartIdx<StreamIdx>::Value +
    									 Node::template StreamSize<StreamIdx>::Value - 1;

    		return Node::Dispatcher::template dispatch<SubstreamIdx>(node->allocator(), std::forward<Fn>(fn), node->is_leaf(), std::forward<Args>(args)...);
    	}

    	template <typename Node, typename Fn, typename... Args>
    	auto treeNode(const Node* node, Fn&& fn, Args&&... args)
    	{
    		constexpr Int SubstreamIdx = Node::template StreamStartIdx<StreamIdx>::Value +
    									 Node::template StreamSize<StreamIdx>::Value - 1;

    		return Node::Dispatcher::template dispatch<SubstreamIdx>(node->allocator(), std::forward<Fn>(fn), node->is_leaf(), std::forward<Args>(args)...);
    	}
    };


    template <Int StreamIdx, typename Fn, typename... Args>
    auto _process_count_substream(NodeBaseG& node, Fn&& fn, Args&&... args)
    {
    	return NodeDispatcher::dispatch(
    			node,
				ProcessCountSubstreamFn<StreamIdx>(),
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    auto _process_count_substream(const NodeBaseG& node, Fn&& fn, Args&&... args) const
    {
    	return NodeDispatcher::dispatch(
    			node,
				ProcessCountSubstreamFn<StreamIdx>(),
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
    }



    struct ProcessCountSubstreamsFn {
    	template <Int StreamIdx, typename CtrT, typename... Args2>
    	auto process(CtrT&& ctr, NodeBaseG& node, Args2&&... args)
    	{
    		return ctr.template _process_count_substream<StreamIdx>(node, std::forward<Args2>(args)...);
    	}

    	template <Int StreamIdx, typename CtrT, typename... Args2>
    	auto process(CtrT&& ctr, const NodeBaseG& node, Args2&&... args)
    	{
    		return ctr.template _process_count_substream<StreamIdx>(node, std::forward<Args2>(args)...);
    	}
    };

    template <typename Fn, typename... Args>
    auto process_count_substreams(NodeBaseG& node, Int stream, Fn&& fn, Args&&... args)
    {
    	return bt::ForEachStream<Streams - 2>::process(
    			stream,
				ProcessCountSubstreamsFn(),
				self(),
				node,
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
    }

    template <typename Fn, typename... Args>
    auto process_count_substreams(const NodeBaseG& node, Int stream, Fn&& fn, Args&&... args) const
    {
    	return bt::ForEachStream<Streams - 2>::process(
    			stream,
				ProcessCountSubstreamsFn(),
				self(),
				node,
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
    }

    struct AddToStreamCounter {
    	template <typename Stream>
    	void stream(Stream* obj, bool leaf, Int idx, CtrSizeT value)
    	{
    		auto block = leaf? 0 : (Stream::Blocks - 1);
    		obj->addValue(block, idx, value);
    	}

    	template <typename Stream>
    	void stream(const Stream* obj, bool leaf, Int idx, CtrSizeT value)
    	{
    		throw vapi::Exception(MA_SRC, "Incorrect static method dispatching");
    	}
    };

    //FIXME: handle PackedOOM correctly for branch nodes
    void add_to_stream_counter(NodeBaseG node, Int stream, Int idx, CtrSizeT value)
    {
    	auto& self = this->self();

    	if (value != 0)
    	{
    		AddToStreamCounter fn;

    		self.process_count_substreams(node, stream, fn, idx, value);

    		while (node->parent_id().isSet())
    		{
    			NodeBaseG parent = self.getNodeParentForUpdate(node);
    			Int parent_idx 	 = node->parent_idx();

    			self.process_count_substreams(parent, stream, fn, parent_idx, value);

    			node = parent;
    		}
    	}
    }

    struct GetStreamCounter {
    	template <typename Stream>
    	auto stream(const Stream* obj, bool leaf, Int idx)
    	{
    		auto block = leaf? 0 : (Stream::Blocks - 1);
    		return obj->getValue(block, idx);
    	}
    };

    CtrSizeT get_stream_counter(const NodeBaseG& node, Int stream, Int idx) const
    {
    	auto& self = this->self();
    	return self.process_count_substreams(node, stream, GetStreamCounter(), idx);
    }

    template <Int StreamIdx>
    CtrSizeT _get_stream_counter(const NodeBaseG& node, Int idx) const
    {
    	auto& self = this->self();
    	return self.template _process_count_substream<StreamIdx>(node, GetStreamCounter(), idx);
    }




    struct SetStreamCounter {
    	template <typename Stream>
    	auto stream(Stream* obj, bool leaf, Int idx, CtrSizeT value)
    	{
    		auto block = leaf? 0 : (Stream::Blocks - 1);
    		auto current_value = obj->getValue(block, idx);

    		obj->setValue(block, idx, value);

    		return current_value;
    	}
    };

    void set_stream_counter(NodeBaseG& node, Int stream, Int idx, CtrSizeT value)
    {
    	auto& self = this->self();
    	self.process_count_substreams(node, stream, SetStreamCounter(), idx, value);
    }

    struct FindOffsetFn {
    	template <typename Stream>
    	auto stream(const Stream* substream, bool leaf, Int idx)
    	{
    		auto block = leaf? 0 : (Stream::Blocks - 1);

			auto result = substream->findGTForward(block, 0, idx);
			return result.idx();
    	}
    };



    Int find_offset(const NodeBaseG& node, Int stream, Int idx) const
    {
    	MEMORIA_ASSERT_TRUE(stream >= 0 && stream < Streams - 1);
    	MEMORIA_ASSERT(idx, >=, 0);

    	return self().process_count_substreams(node, stream, FindOffsetFn(), idx);
    }



    struct CountStreamItemsFn {
    	template <typename Stream>
    	auto stream(const Stream* substream, bool leaf, Int end)
    	{
    		auto block = leaf? 0 : (Stream::Blocks - 1);

    		return substream->sum(block, end);
    	}
    };

    CtrSizeT count_items(const NodeBaseG& node, Int stream, Int end) const
    {
    	return self().process_count_substreams(node, stream, CountStreamItemsFn(), end);
    }

    Position count_items(const NodeBaseG& node) const
    {
    	auto& self = this->self();
    	auto sizes = self.getNodeSizes(node);

    	Position counts;

    	counts[0] = sizes[0];

    	for (Int s = 1; s < Streams; s++)
    	{
    		counts[s] = self.count_items(node, s - 1, sizes[s - 1]);
    	}

    	return counts;
    }

    Position total_counts() const
    {
    	auto& self = this->self();

    	NodeBaseG root = self.getRoot();

    	auto root_counts = self.count_items(root);

    	root_counts[0] = self.sizes()[0];

    	return root_counts;
    }


    void compute_leaf_prefixes(const NodeBaseG leaf, const Position& extent, LeafPrefixRanks& prefixes) const
    {
    	const auto& self  = this->self();

    	prefixes[Streams - 2][Streams - 1] = extent[Streams - 1];

    	for (Int s = SearchableStreams - 1; s > 0; s--)
    	{
    		prefixes[s - 1] = prefixes[s];
    		self.count_downstream_items(leaf, prefixes[s], prefixes[s - 1], s, extent[s]);
    		prefixes[s - 1][s] = extent[s];
    	}
    }


    Position leaf_rank(const NodeBaseG& leaf, Position sizes, const LeafPrefixRanks& prefixes, Int pos) const
    {
    	for (Int s = 0; s < Streams; s++)
    	{
    		Int sum = prefixes[s].sum();
    		if (pos >= sum)
    		{
    			return bttl::detail::ZeroRankHelper<0, Streams>::process(this, leaf, sizes, prefixes[s], pos);
    		}
    		else {
    			sizes[s] = 0;
    		}
    	}

    	return Position();
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

    		return fn.indexes_ + fn.prefix_;
    	}
    };

    template <Int Stream, typename... Args>
    Position _streams_rank(const NodeBaseG& leaf, Args&&... args) const
    {
    	return LeafDispatcher::dispatch(leaf, _StreamsRankFn<Stream>(), std::forward<Args>(args)...);
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


    void count_downstream_items(const NodeBaseG& leaf, const Position& prefix, Position& sums, Int stream, Int end) const
    {
    	return LeafDispatcher::dispatch(leaf, CountStreamsItemsFn(prefix, sums, stream, end));
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bttl::RanksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
