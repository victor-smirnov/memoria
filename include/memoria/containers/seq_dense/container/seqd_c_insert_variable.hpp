
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_INSERT_VARIABLE_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_INSERT_VARIABLE_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrInsertVariableName)

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

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    //==========================================================================================

//    MEMORIA_DECLARE_NODE_FN(LayoutNodeFn, layout);
//    void layoutLeafNode(NodeBaseG& node, Int size) const
//    {
//    	LeafDispatcher::dispatch(node, LayoutNodeFn(), Position(size));
//    }

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
    		obj->insert(buffer, pos, start, size);
    	}
    };


    template <typename LeafPosition, typename Buffer>
    bool doInsertBufferIntoLeaf(NodeBaseG& leaf, PageUpdateMgr& mgr, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
    {
    	try {
    		LeafDispatcher::dispatch(leaf, InsertBufferIntoLeafFn(), pos, start, size - start, buffer);
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

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrInsertVariableName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
