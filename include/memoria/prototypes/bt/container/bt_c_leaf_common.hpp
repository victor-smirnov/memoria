
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

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    using CtrSizeT = typename Types::CtrSizeT;

    static const Int Streams                                                    = Types::Streams;

    MEMORIA_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, BranchNodeEntry);
    BranchNodeEntry split_leaf_node(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
    {
    	return LeafDispatcher::dispatch(src, tgt, SplitNodeFn(), split_at);
    }

    NodeBaseG split_leaf_p(NodeBaseG& left_node, const Position& split_at)
    {
        auto& self = this->self();

        return self.splitP(left_node, [&self, &split_at](NodeBaseG& left, NodeBaseG& right){
            return self.split_leaf_node(left, right, split_at);
        });
    }





    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto apply_substreams_fn(NodeBaseG& leaf, Fn&& fn, Args&&... args)
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto apply_substreams_fn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <Int Stream, typename Fn, typename... Args>
    auto apply_stream_fn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    {
    	return LeafDispatcher::dispatch(leaf, StreamNodeFn<Stream>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto read_substreams(const NodeBaseG& leaf, Args&&... args) const
    {
    	 return self().template apply_substreams_fn<Stream, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }



    template <Int Stream, typename... Args>
    auto read_stream(const NodeBaseG& leaf, Args&&... args) const
    {
    	 return self().template apply_stream_fn<Stream>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }







    struct SumFn {
    	template <typename Stream>
    	auto stream(const Stream* s, Int block, Int from, Int to)
    	{
    		return s->sum(block, from, to);
    	}

    	template <typename Stream>
    	auto stream(const Stream* s, Int from, Int to)
    	{
    		return s->sums(from, to);
    	}
    };

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto _sum(const NodeBaseG& leaf, Args&&... args) const
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), SumFn(), std::forward<Args>(args)...);
    }

    struct FindFn {
    	template <typename Stream, typename... Args>
    	auto stream(const Stream* s, Args&&... args)
    	{
    		return s->findForward(std::forward<Args>(args)...).idx();
    	}
    };

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto find_forward(const NodeBaseG& leaf, Args&&... args) const
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), FindFn(), std::forward<Args>(args)...);
    }



    // ============================================ Insert Data ======================================== //


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



    class InsertDataBlockResult {
    	Position inserted_size_;
    	bool extra_space_;
    public:
    	InsertDataBlockResult(Position size, bool extra_space): inserted_size_(size), extra_space_(extra_space){}

    	const Position& inserted_size() const {return inserted_size_;}
    	bool has_extra_space() const {return extra_space_;}
    };


    template <typename Provider>
    Position insert_data_into_leaf(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
    	auto& self = this->self();

    	self.updatePageG(leaf);

    	self.layoutLeafNode(leaf, Position(0));

    	if (provider.hasData())
    	{
    		auto end = provider.fill(leaf, pos);

    		return end;
    	}

    	return pos;
    }



    class InsertDataResult {
    	NodeBaseG leaf_;
    	Position position_;
    public:
    	InsertDataResult(NodeBaseG leaf, const Position& position = Position()): leaf_(leaf), position_(position){}

    	NodeBaseG& leaf() {return leaf_;}
    	const NodeBaseG& leaf() const {return leaf_;}

    	const Position& position() const {return position_;}
    	Position& position() {return position_;}
    };

    template <typename Provider>
    InsertDataResult insert_provided_data(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
    	auto& self = this->self();

    	auto last_pos = self.insert_data_into_leaf(leaf, pos, provider);

    	if (provider.hasData())
    	{
    		// has to be defined in subclasses
    		if (!self.isAtTheEnd2(leaf, last_pos))
    		{
    			auto next_leaf = self.split_leaf_p(leaf, last_pos);

    			self.insert_data_into_leaf(leaf, last_pos, provider);

    			provider.nextLeaf(leaf);

    			if (provider.hasData())
    			{
    				return insertDataRest(leaf, next_leaf, provider);
    			}
    			else {
    				return InsertDataResult(next_leaf, Position());
    			}
    		}
    		else {
    			provider.nextLeaf(leaf);

    			auto next_leaf = self.getNextNodeP(leaf);

    			if (next_leaf.isSet())
    			{
    				return insertDataRest(leaf, next_leaf, provider);
    			}
    			else {
    				return insertDataRest(leaf, provider);
    			}
    		}
    	}
    	else {
    		return InsertDataResult(leaf, last_pos);
    	}
    }



    template <typename Provider>
    LeafList createLeafDataList(Provider& provider)
    {
        auto& self = this->self();

        CtrSizeT    total = 0;
        NodeBaseG   head;
        NodeBaseG   current;

        Int page_size = self.getRootMetadata().page_size();

        while (provider.hasData())
        {
        	NodeBaseG node = self.createNode(0, false, true, page_size);

        	if (head.isSet())
        	{
        		current->next_leaf_id() = node->id();
        	}
        	else {
        		head = node;
        	}

        	self.insert_data_into_leaf(node, Position(), provider);

        	provider.nextLeaf(node);

        	current = node;

        	total++;
        }

        total += provider.orphan_splits();

        return LeafList(total, head, current);
    }




    template <typename Provider>
    InsertDataResult insertDataRest(NodeBaseG& leaf, NodeBaseG& next_leaf, Provider& provider)
    {
    	auto& self = this->self();

    	provider.split_watcher().first = leaf;

    	auto leaf_list = self.createLeafDataList(provider);

    	if (provider.split_watcher().second.isSet())
    	{
    		leaf = provider.split_watcher().second;
    	}

    	Int path_parent_idx = leaf->parent_idx() + 1;

    	if (leaf_list.size() > 0)
    	{
    		using LeafListProvider = typename Base::ListLeafProvider;

    		LeafListProvider list_provider(self, leaf_list.head(), leaf_list.size());

    		NodeBaseG parent = self.getNodeParentForUpdate(leaf);

    		self.insert_subtree(parent, path_parent_idx, list_provider);

    		auto& last_leaf = leaf_list.tail();

    		auto last_leaf_size = self.getLeafStreamSizes(last_leaf);

    		if (self.mergeLeafNodes(last_leaf, next_leaf, [](const Position&){}))
    		{
    			return InsertDataResult(last_leaf, last_leaf_size);
    		}
    		else {
    			return InsertDataResult(next_leaf);
    		}
    	}
    	else {
    		return InsertDataResult(next_leaf);
    	}
    }


    template <typename Provider>
    InsertDataResult insertDataRest(NodeBaseG& leaf, Provider& provider)
    {
    	auto& self = this->self();

    	if (leaf->is_root())
    	{
    		self.newRootP(leaf);
    	}

    	provider.split_watcher().first = leaf;

    	auto leaf_list = self.createLeafDataList(provider);

    	if (provider.split_watcher().second.isSet())
    	{
    		leaf = provider.split_watcher().second;
    	}

    	Int path_parent_idx = leaf->parent_idx() + 1;

    	if (leaf_list.size() > 0)
    	{
    		using LeafListProvider = typename Base::ListLeafProvider;

    		LeafListProvider list_provider(self, leaf_list.head(), leaf_list.size());

    		NodeBaseG parent = self.getNodeParentForUpdate(leaf);

    		self.insert_subtree(parent, path_parent_idx, list_provider);

    		auto& last_leaf = leaf_list.tail();

    		auto last_leaf_size = self.getLeafStreamSizes(last_leaf);

    		return InsertDataResult(last_leaf, last_leaf_size);
    	}
    	else {
    		auto leaf_size = self.getLeafStreamSizes(leaf);
    		return InsertDataResult(leaf, leaf_size);
    	}
    }

    template <typename Fn, typename... Args>
    SplitStatus updateAtomic(Iterator& iter, Fn&& fn, Args&&... args)
    {
    	auto& self = this->self();

    	PageUpdateMgr mgr(self);

    	self.updatePageG(iter.leaf());

    	mgr.add(iter.leaf());

    	try {
    		LeafDispatcher::dispatch(
    				iter.leaf(),
					fn,
					std::forward<Args>(args)...
    		);

    		return SplitStatus::NONE;
    	}
    	catch (PackedOOMException& e)
    	{
    		mgr.rollback();

    		SplitStatus status = iter.split();

    		LeafDispatcher::dispatch(
    				iter.leaf(),
					fn,
					std::forward<Args>(args)...
    		);

    		return status;
    	}
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS






#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
