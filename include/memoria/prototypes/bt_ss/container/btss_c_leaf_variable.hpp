
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_MODEL_LEAF_VARIABLE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_MODEL_LEAF_VARIABLE_HPP

#include <memoria/prototypes/bt_ss/btss_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btss::LeafVariableName)

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

    typedef std::function<Accumulator (NodeBaseG&, NodeBaseG&)>                 SplitFn;
    typedef std::function<void (const Position&, Int)>                          MergeFn;

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
MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafVariableName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
