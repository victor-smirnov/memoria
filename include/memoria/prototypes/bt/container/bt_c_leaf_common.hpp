
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_COMMON_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_COMMON_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::LeafCommonName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    using CtrSizeT = typename Types::CtrSizeT;

    static const Int Streams                                                    = Types::Streams;

    template <Int Stream>
    using StreamInputTuple = typename Types::template StreamInputTuple<Stream>;


    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, GetLeafValuesFn, Args...>;


    NodeBaseG splitLeafP(NodeBaseG& left_node, const Position& split_at)
    {
        auto& self = this->self();

        return self.splitP(left_node, [&self, &split_at](NodeBaseG& left, NodeBaseG& right){
            return self.splitLeafNode(left, right, split_at);
        });
    }


    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto _readLeafEntry(const NodeBaseG& leaf, Args&&... args) const -> ReadLeafEntryRtnType<Stream, SubstreamsIdxList, Args...>
    {
    	 return self().template _applySubstreamsFn<Stream, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }



    MEMORIA_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, Accumulator);
    Accumulator splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at);



    //==============================================================================================

    template <typename LeafPosition>
    class InsertBufferResult {
    	LeafPosition inserted_size_;
    	bool extra_space_;
    public:
    	InsertBufferResult(LeafPosition size, bool extra_space): inserted_size_(size), extra_space_(extra_space){}

    	const LeafPosition& inserted_size() const {return inserted_size_;}
    	bool has_extra_space() const {return extra_space_;}
    };


    template <typename LeafPosition, typename Buffer>
    LeafPosition insertBuffersIntoLeaf(NodeBaseG& leaf, LeafPosition pos, InputBufferProvider<LeafPosition, Buffer>& provider)
    {
    	auto& self = this->self();

    	self.updatePageG(leaf);

    	self.layoutLeafNode(leaf, 0);

    	auto first_pos = pos;

    	while (provider.hasData())
    	{
    		if (provider.isConsumed())
    		{
    			provider.nextBuffer();
    		}

    		const Buffer* buffer = provider.buffer();
    		auto start 	= provider.start();

    		auto inserted = self.insertBufferIntoLeaf(leaf, pos, start, provider.size(), buffer);

    		auto inserted_size = inserted.inserted_size();

    		pos += inserted_size;

    		provider.consumed(inserted_size);

    		if (!inserted.has_extra_space()) {
    			break;
    		}
    	}

    	if (leaf->parent_id().isSet())
    	{
    		auto sums = self.sums(leaf, Position(first_pos), Position(pos));

    		self.updateParent(leaf, sums);
    	}

    	return pos;
    }

    template <typename LeafPosition>
    bool isAtTheEnd(const NodeBaseG& leaf, LeafPosition pos)
    {
    	Int size = self().template getLeafStreamSize<0>(leaf);
    	return pos >= size;
    }




    template <typename LeafPosition>
    class InsertBuffersResult {
    	NodeBaseG leaf_;
    	LeafPosition position_;
    public:
    	InsertBuffersResult(NodeBaseG leaf, LeafPosition position): leaf_(leaf), position_(position){}

    	NodeBaseG& leaf() {return leaf_;}
    	const NodeBaseG& leaf() const {return leaf_;}

    	const LeafPosition& position() const {return position_;}
    	LeafPosition& position() {return position_;}
    };

    template <typename LeafPosition, typename Buffer>
    InsertBuffersResult<LeafPosition> insertBuffers(NodeBaseG& leaf, LeafPosition pos, InputBufferProvider<LeafPosition, Buffer>& provider)
    {
    	auto& self = this->self();

    	auto last_pos = self.insertBuffersIntoLeaf(leaf, pos, provider);

    	if (provider.hasData())
    	{
    		if (!self.isAtTheEnd(leaf, last_pos))
    		{
    			auto next_leaf = self.splitLeafP(leaf, Position(last_pos));

    			self.insertBuffersIntoLeaf(leaf, last_pos, provider);

    			if (provider.hasData())
    			{
    				return insertBuffersRest(leaf, next_leaf, provider);
    			}
    			else {
    				return InsertBuffersResult<LeafPosition>(next_leaf, provider.zero());
    			}
    		}
    		else {
    			auto next_leaf = self.getNextNodeP(leaf);

    			if (next_leaf.isSet())
    			{
    				return insertBuffersRest(leaf, next_leaf, provider);
    			}
    			else {
    				return insertBuffersRest(leaf, provider);
    			}
    		}
    	}
    	else {
    		return InsertBuffersResult<LeafPosition>(leaf, last_pos);
    	}
    }

    class LeafList {
    	CtrSizeT size_;
    	NodeBaseG head_;
    	NodeBaseG tail_;
    public:
    	LeafList(CtrSizeT size, NodeBaseG head, NodeBaseG tail): size_(size), head_(head), tail_(tail) {}

    	CtrSizeT size() const {return size_;}
    	const NodeBaseG& head() const {return head_;}
    	const NodeBaseG& tail() const {return tail_;}

    	NodeBaseG& head() {return head_;}
    	NodeBaseG& tail() {return tail_;}
    };

    template <typename LeafPosition, typename Buffer>
    LeafList createLeafList3(InputBufferProvider<LeafPosition, Buffer>& provider);

private:

    template <typename LeafPosition, typename Buffer>
    InsertBuffersResult<LeafPosition> insertBuffersRest(NodeBaseG& leaf, NodeBaseG& next_leaf, InputBufferProvider<LeafPosition, Buffer>& provider)
    {
    	auto& self = this->self();

    	Int path_parent_idx = leaf->parent_idx() + 1;

    	auto leaf_list = self.createLeafList3(provider);

    	using Provider = typename Base::ListLeafProvider;

    	Provider list_provider(self, leaf_list.head(), leaf_list.size());

    	NodeBaseG parent = self.getNodeParentForUpdate(leaf);

    	self.insert_subtree(parent, path_parent_idx, list_provider);

    	auto& last_leaf = leaf_list.tail();

    	auto last_leaf_size = self.getLeafStreamSizes(last_leaf);

    	if (self.mergeLeafNodes(last_leaf, next_leaf, [](const Position&){}))
    	{
    		return InsertBuffersResult<LeafPosition>(last_leaf, last_leaf_size.get());
    	}
    	else {
    		return InsertBuffersResult<LeafPosition>(next_leaf, provider.zero());
    	}
    }


    template <typename LeafPosition, typename Buffer>
    InsertBuffersResult<LeafPosition> insertBuffersRest(NodeBaseG& leaf, InputBufferProvider<LeafPosition, Buffer>& provider)
    {
    	auto& self = this->self();

    	if (leaf->is_root()) {
    		self.newRootP(leaf);
    	}

    	Int path_parent_idx = leaf->parent_idx() + 1;

    	auto leaf_list = self.createLeafList3(provider);

    	using Provider = typename Base::ListLeafProvider;

    	Provider list_provider(self, leaf_list.head(), leaf_list.size());

    	NodeBaseG parent = self.getNodeParentForUpdate(leaf);

    	self.insert_subtree(parent, path_parent_idx, list_provider);

    	auto& last_leaf = leaf_list.tail();

    	auto last_leaf_size = self.getLeafStreamSizes(last_leaf);

    	return InsertBuffersResult<LeafPosition>(last_leaf, last_leaf_size.get());
    }



MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
{
    return LeafDispatcher::dispatch(src, tgt, SplitNodeFn(), split_at);
}




M_PARAMS
template <typename LeafPosition, typename Buffer>
typename M_TYPE::LeafList M_TYPE::createLeafList3(InputBufferProvider<LeafPosition, Buffer>& provider)
{
    auto& self = this->self();

    CtrSizeT    total   = 0;
    NodeBaseG   head;
    NodeBaseG   current;

    Int page_size = self.getRootMetadata().page_size();

    while (provider.hasData())
    {
    	total++;

    	NodeBaseG node = self.createNode1(0, false, true, page_size);

    	self.insertBuffersIntoLeaf(node, provider.zero(), provider);

    	if (head.isSet())
    	{
    		current->next_leaf_id() = node->id();
    		current                 = node;
    	}
    	else {
    		head = current = node;
    	}
    }

    return LeafList(total, head, current);
}




#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
