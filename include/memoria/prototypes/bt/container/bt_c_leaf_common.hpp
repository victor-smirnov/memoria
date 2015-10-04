
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
    using ReadLeafStreamEntryRtnType = DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, GetLeafValuesFn, Args...>;

    NodeBaseG splitLeafP(NodeBaseG& left_node, const Position& split_at)
    {
        auto& self = this->self();

        return self.splitP(left_node, [&self, &split_at](NodeBaseG& left, NodeBaseG& right){
            return self.splitLeafNode(left, right, split_at);
        });
    }

    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto _applySubstreamsFn(NodeBaseG& leaf, Fn&& fn, Args&&... args)
    -> DispatchRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, Fn, Args...>
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto _applySubstreamsFn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    -> DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, Fn, Args...>
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto _readLeafStreamEntry(const NodeBaseG& leaf, Args&&... args) const -> ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>
    {
    	 return self().template _applySubstreamsFn<Stream, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }



    MEMORIA_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, Accumulator);
    Accumulator splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at);


    struct SumFn {
    	template <typename Stream>
    	auto stream(const Stream* s, Int block, Int from, Int to) -> decltype(s->sum(block, from, to))
    	{
    		return s->sum(block, from, to);
    	}

    	template <typename Stream>
    	auto stream(const Stream* s, Int from, Int to) -> decltype(s->sums(from, to))
    	{
    		return s->sums(from, to);
    	}
    };

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto _sum(const NodeBaseG& leaf, Args&&... args) const
    -> DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, SumFn, Args...>
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), SumFn(), std::forward<Args>(args)...);
    }

    struct FindFn {
    	template <typename Stream, typename... Args>
    	auto stream(const Stream* s, Args&&... args) -> decltype(s->findForward(args...).idx())
    	{
    		return s->findForward(std::forward<Args>(args)...).idx();
    	}
    };

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto _find(const NodeBaseG& leaf, Args&&... args) const
    -> DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, FindFn, Args...>
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), FindFn(), std::forward<Args>(args)...);
    }


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

    	self.layoutLeafNode(leaf, Position(0));

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
    LeafList createLeafList(InputBufferProvider<LeafPosition, Buffer>& provider);


    // ============================================ Insert Data ======================================== //



    class InsertDataBlockResult {
    	Position inserted_size_;
    	bool extra_space_;
    public:
    	InsertDataBlockResult(Position size, bool extra_space): inserted_size_(size), extra_space_(extra_space){}

    	const Position& inserted_size() const {return inserted_size_;}
    	bool has_extra_space() const {return extra_space_;}
    };


    template <typename Provider>
    Position insertDataIntoLeaf(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
    	auto& self = this->self();

    	self.updatePageG(leaf);

    	self.layoutLeafNode(leaf, Position(0));

    	if (provider.hasData())
    	{
    		auto end = self.fillLeaf(leaf, pos, provider);

    		return end;
    	}

    	return pos;
    }


    template <typename Provider>
    Position fillLeaf(NodeBaseG& leaf, const Position& pos, Provider& provider) {
    	return provider.fill(leaf, pos);
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
    InsertDataResult insertData(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
    	auto& self = this->self();

    	provider.prepare(leaf, pos);

    	auto last_pos = self.insertDataIntoLeaf(leaf, pos, provider);

    	if (provider.hasData())
    	{
    		// has to be defined in subclasses
    		if (!self.isAtTheEnd2(leaf, last_pos))
    		{
    			auto next_leaf = self.splitLeafP(leaf, last_pos);

    			self.insertDataIntoLeaf(leaf, last_pos, provider);

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
    LeafList createLeafDataList(Provider& provider);

    // ============================================ Insert Data ======================================== //

    template <typename LeafPosition, typename Buffer>
    InsertBuffersResult<LeafPosition> insertBuffersRest(NodeBaseG& leaf, NodeBaseG& next_leaf, InputBufferProvider<LeafPosition, Buffer>& provider)
    {
    	auto& self = this->self();

    	Int path_parent_idx = leaf->parent_idx() + 1;

    	auto leaf_list = self.createLeafList(provider);

    	if (leaf_list.size() > 0)
    	{
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
    	else {
    		return InsertBuffersResult<LeafPosition>(next_leaf, provider.zero());
    	}
    }


    template <typename LeafPosition, typename Buffer>
    InsertBuffersResult<LeafPosition> insertBuffersRest(NodeBaseG& leaf, InputBufferProvider<LeafPosition, Buffer>& provider)
    {
    	auto& self = this->self();

    	if (leaf->is_root())
    	{
    		self.newRootP(leaf);
    	}

    	Int path_parent_idx = leaf->parent_idx() + 1;

    	auto leaf_list = self.createLeafList(provider);

    	if (leaf_list.size() > 0)
    	{
    		using Provider = typename Base::ListLeafProvider;

    		Provider list_provider(self, leaf_list.head(), leaf_list.size());

    		NodeBaseG parent = self.getNodeParentForUpdate(leaf);

    		self.insert_subtree(parent, path_parent_idx, list_provider);

    		auto& last_leaf = leaf_list.tail();

    		auto last_leaf_size = self.getLeafStreamSizes(last_leaf);

    		return InsertBuffersResult<LeafPosition>(last_leaf, last_leaf_size.get());
    	}
    	else {
    		auto leaf_size = self.getLeafStreamSizes(leaf);
    		return InsertBuffersResult<LeafPosition>(leaf, leaf_size.get());
    	}
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


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
{
    return LeafDispatcher::dispatch(src, tgt, SplitNodeFn(), split_at);
}




M_PARAMS
template <typename LeafPosition, typename Buffer>
typename M_TYPE::LeafList M_TYPE::createLeafList(InputBufferProvider<LeafPosition, Buffer>& provider)
{
    auto& self = this->self();

    CtrSizeT    total = 0;
    NodeBaseG   head;
    NodeBaseG   current;

    Int page_size = self.getRootMetadata().page_size();

    while (provider.hasData())
    {
    	NodeBaseG node = self.createNode1(0, false, true, page_size);

    	self.insertBuffersIntoLeaf(node, provider.zero(), provider);

    	if (!self.isEmpty(node))
    	{
    		total++;

    		if (head.isSet())
    		{
    			current->next_leaf_id() = node->id();
    			current                 = node;
    		}
    		else {
    			head = current = node;
    		}
    	}
    	else {
    		self.allocator().removePage(node->id(), self.master_name());
    	}
    }

    return LeafList(total, head, current);
}



M_PARAMS
template <typename Provider>
typename M_TYPE::LeafList M_TYPE::createLeafDataList(Provider& provider)
{
    auto& self = this->self();

    CtrSizeT    total = 0;
    NodeBaseG   head;
    NodeBaseG   current;

    Int page_size = self.getRootMetadata().page_size();

    while (provider.hasData())
    {
    	NodeBaseG node = self.createNode1(0, false, true, page_size);

    	if (head.isSet())
    	{
    		current->next_leaf_id() = node->id();
    	}
    	else {
    		head = node;
    	}

    	self.insertDataIntoLeaf(node, Position(), provider);

    	provider.nextLeaf(node);

    	current = node;

    	total++;
    }

    total += provider.orphan_splits();

    return LeafList(total, head, current);
}


#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
