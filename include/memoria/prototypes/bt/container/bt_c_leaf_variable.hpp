
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_VARIABLE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_VARIABLE_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::LeafVariableName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef std::function<void (const Position&)>                          		MergeFn;

    typedef typename Types::Source                                              Source;

    static const Int Streams                                                    = Types::Streams;

    template <Int Stream>
    using StreamInputTuple = typename Types::template StreamInputTuple<Stream>;


    struct InsertEntryIntoStreamHanlder
    {
    	template <
    		Int Offset,
    	    bool StreamStart,
    	    Int Idx,
    		typename SubstreamType,
    		typename AccumulatorItem,
    		typename Entry
    	>
    	void stream(SubstreamType* obj, AccumulatorItem& accum, Int idx, const Entry& entry)
    	{
    		obj->template _insert<Offset>(idx, std::get<Idx>(entry), accum);

    		if (StreamStart)
    		{
    			accum[0] += 1;
    		}
    	}
    };




    template <Int Stream>
    struct InsertEntryIntoStreamFn
    {
    	template <typename NTypes, typename... Args>
    	void treeNode(LeafNode<NTypes>* node, Int idx, Accumulator& accum, Args&&... args)
    	{
    		node->layout(255);
    		node->template processStreamAcc<Stream>(InsertEntryIntoStreamHanlder(), accum, idx, std::forward<Args>(args)...);
    	}
    };




    template <Int Stream>
    std::tuple<bool, Accumulator> tryInsertStreamEntry(Iterator& iter, const StreamInputTuple<Stream>& entry)
    {
    	auto& self = this->self();

    	PageUpdateMgr mgr(self);

    	self.updatePageG(iter.leaf());

    	mgr.add(iter.leaf());

    	try {
    		Accumulator accum;
    		LeafDispatcher::dispatch(iter.leaf(), InsertEntryIntoStreamFn<Stream>(), iter.idx(), accum, entry);
    		return std::make_tuple(true, accum);
    	}
    	catch (PackedOOMException& e)
    	{
    		mgr.rollback();
    		return std::make_tuple(false, Accumulator());
    	}
    }






    //=========================================================================================

     struct UpdateStreamEntryHanlder
     {
     	template <
     		Int Offset,
     		bool Start,
     		Int Idx,
     		typename SubstreamType,
     		typename AccumulatorItem,
     		typename Entry
     	>
     	void stream(SubstreamType* obj, AccumulatorItem& accum, Int idx, const Entry& entry)
     	{
     		obj->template _update<Offset>(idx, std::get<Idx>(entry), accum);
     	}
     };

     template <Int Stream, typename SubstreamsList>
     struct UpdateStreamEntryFn
     {
     	template <typename NTypes, typename... Args>
     	void treeNode(LeafNode<NTypes>* node, Int idx, Accumulator& accum, Args&&... args)
     	{
     		node->template processSubstreamsByIdxAcc<
     			Stream,
     			SubstreamsList
     		>(
     			UpdateStreamEntryHanlder(),
     			accum,
     			idx,
     			std::forward<Args>(args)...
     		);
     	}
     };


     template <Int Stream, typename SubstreamsList, typename... TupleTypes>
     std::tuple<bool, Accumulator> tryUpdateStreamEntry(Iterator& iter, const std::tuple<TupleTypes...>& entry)
     {
     	static_assert(
     			ListSize<SubstreamsList>::Value == sizeof...(TupleTypes),
     			"Input tuple size must match SubstreamsList size"
     	);

     	auto& self = this->self();

     	PageUpdateMgr mgr(self);

     	self.updatePageG(iter.leaf());

     	mgr.add(iter.leaf());

     	try {
     		Accumulator accum;
     		LeafDispatcher::dispatch(
     				iter.leaf(),
     				UpdateStreamEntryFn<Stream, SubstreamsList>(),
     				iter.idx(),
     				accum,
     				entry
     		);
     		return std::make_tuple(true, accum);
     	}
     	catch (PackedOOMException& e)
     	{
     		mgr.rollback();
     		return std::make_tuple(false, Accumulator());
     	}
     }



     //==========================================================================================


     template <typename LeafPosition>
     using InsertBufferResult = typename Base::template InsertBufferResult<LeafPosition>;

     template <typename LeafPosition, typename Buffer>
     InsertBufferResult<LeafPosition> insertBufferIntoLeaf(NodeBaseG& leaf, LeafPosition pos, LeafPosition start, LeafPosition end, const Buffer* buffer)
     {
    	 auto& self = this->self();

    	 PageUpdateMgr mgr(self);

    	 mgr.add(leaf);

    	 if (end - start > leaf->page_size() * 8) {
    		 end = start + leaf->page_size() * 8;
    	 }

    	 if (self.doInsertBufferIntoLeaf(leaf, mgr, pos, start, end, buffer))
    	 {
    		 return InsertBufferResult<LeafPosition>(end - start, true);
    	 }
    	 else if (end - start <= 1)
    	 {
    		 return InsertBufferResult<LeafPosition>(0, true);
    	 }
    	 else {
    		 LeafPosition imax = end, imin = start;
    		 LeafPosition inserted = 0;
    		 Int accepts 	 = 0;

    		 while (accepts < 5 && imax > imin)
    		 {
    			 if (imax - 1 != imin)
    			 {
    				 int mid = imin + ((imax - imin) / 2);

    				 if (self.doInsertBufferIntoLeaf(leaf, mgr, pos + inserted, start + inserted, start + inserted + mid - imin, buffer))
    				 {
    					 accepts++;
    					 inserted += mid - imin;
    					 imin = mid + 1;
    				 }
    				 else {
    					 imax = mid - 1;
    				 }
    			 }
    			 else {
    				 if (self.doInsertBufferIntoLeaf(leaf, mgr, pos + inserted, start + inserted, start + inserted + 1, buffer))
    				 {
    					 accepts++;
    					 inserted += 1;
    				 }
    				 break;
    			 }
    		 }

    		 return InsertBufferResult<LeafPosition>(inserted, false);
    	 }
     }



     struct InsertBufferIntoLeafFn
     {
     	template <typename NTypes, typename LeafPosition, typename Buffer>
     	void treeNode(LeafNode<NTypes>* node, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
     	{
     		node->processAll(*this, pos, start, size, buffer);
     	}

     	template <typename StreamType, typename LeafPosition, typename Buffer>
     	void stream(StreamType* obj, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
     	{
     		obj->insert(pos, start, size, buffer);
     	}
     };


     template <typename LeafPosition, typename Buffer>
     bool doInsertBufferIntoLeaf(NodeBaseG& leaf, PageUpdateMgr& mgr, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
     {
    	 try {
    		 LeafDispatcher::dispatch(leaf, InsertBufferIntoLeafFn(), pos, start, size, buffer);
    		 mgr.checkpoint(leaf);
    		 return true;
    	 }
    	 catch (PackedOOMException& ex)
    	 {
    		 mgr.restoreNodeState();
    		 return false;
    	 }
     }

     NodeBaseG createNextLeaf(NodeBaseG& leaf);

     MEMORIA_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
     bool tryMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn = [](const Position&){});
     bool mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});
     bool mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafVariableName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::createNextLeaf(NodeBaseG& left_node)
{
    auto& self = this->self();

    if (left_node->is_root())
    {
        self.newRootP(left_node);
    }
    else {
        self.updatePageG(left_node);
    }

    NodeBaseG left_parent  = self.getNodeParentForUpdate(left_node);

    NodeBaseG other  = self.createNode1(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

    Accumulator sums;

    Int parent_idx = left_node->parent_idx();

    PageUpdateMgr mgr(self);
    mgr.add(left_parent);

    try {
        self.insertNonLeafP(left_parent, parent_idx + 1, sums, other->id());
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();

        NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

        mgr.add(right_parent);

        try {
            self.insertNonLeafP(right_parent, 0, sums, other->id());
        }
        catch (PackedOOMException ex2)
        {
            mgr.rollback();

            Int right_parent_size = self.getNodeSize(right_parent, 0);

            splitPathP(right_parent, right_parent_size / 2);

            self.insertNonLeafP(right_parent, 0, sums, other->id());
        }
    }
    catch (Exception& ex) {
        cout<<ex<<endl;
        throw;
    }

    return other;
}


M_PARAMS
bool M_TYPE::tryMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);

    self.updatePageG(src);
    self.updatePageG(tgt);

    mgr.add(src);
    mgr.add(tgt);

    Position tgt_sizes  = self.getNodeSizes(tgt);

    try {
        Int tgt_size            = self.getNodeSize(tgt, 0);
        NodeBaseG src_parent    = self.getNodeParent(src);
        Int parent_idx          = src->parent_idx();

        MEMORIA_ASSERT(parent_idx, >, 0);

        LeafDispatcher::dispatch(src, tgt, TryMergeNodesFn());

        self.updateChildren(tgt, tgt_size);

        Accumulator sums        = self.sums(src_parent, parent_idx, parent_idx + 1);

        self.removeNonLeafNodeEntry(src_parent, parent_idx);

        Int idx = parent_idx - 1;

        self.updatePath(src_parent, idx, sums);

        self.allocator().removePage(src->id(), self.master_name());

        fn(tgt_sizes);

        return true;
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();
    }

    return false;
}


M_PARAMS
bool M_TYPE::mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.isTheSameParent(tgt, src))
    {
        return self.mergeCurrentLeafNodes(tgt, src, fn);
    }
    else
    {
        NodeBaseG tgt_parent = self.getNodeParent(tgt);
        NodeBaseG src_parent = self.getNodeParent(src);

        if (self.mergeBranchNodes(tgt_parent, src_parent))
        {
            return self.mergeCurrentLeafNodes(tgt, src, fn);
        }
        else
        {
            return false;
        }
    }
}




M_PARAMS
bool M_TYPE::mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.tryMergeLeafNodes(tgt, src, fn))
    {
        self.removeRedundantRootP(tgt);
        return true;
    }
    else {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
