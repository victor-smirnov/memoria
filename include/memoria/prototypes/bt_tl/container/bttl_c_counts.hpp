
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTTL_CTR_COUNTS_HPP
#define _MEMORIA_PROTOTYPES_BTTL_CTR_COUNTS_HPP


#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bttl::CountsName)

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

    static const Int Streams = Types::Streams;

    using PageUpdateMgt 	= typename Types::PageUpdateMgr;


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

    Position total_counts() const
    {
    	auto& self = this->self();

    	NodeBaseG root = self.getRoot();

    	Position totals;

    	Position sizes = self.getNodeSizes(root);

    	totals[0] = self.sizes()[0];

    	for (Int s = 1; s < Streams; s++)
    	{
    		totals[s] = self.count_items(root, s - 1, sizes[s - 1]);
    	}

    	return totals;
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bttl::CountsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
